#include "EventLoop.h"
#include "Poller.h"
#include "Logger.h"
#include "Channel.h"
#include <sys/eventfd.h>
#include <memory>

__thread EventLoop* t_loopInThisThread = nullptr; //thread local

const int kPollTimeMs = 10000; /*默认poller的超时时间*/

/**
 * @brief  创建wakeup fd,用来唤醒subreactor处理新加入的channel
 * @return int: 
 */
int creatEventFd(){
    int evtfd = ::eventfd(0, EFD_NONBLOCK|EFD_CLOEXEC);
    if (evtfd < 0) {
        LOG_FATAL("eventfd error:%d\n", errno);
    }
    return evtfd;
}

/**
 * @brief  @File Name: Event Loop:: Event Loop
 * 构造Eventloop
 */
EventLoop::EventLoop() 
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(creatEventFd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
    , currentActiveChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d \n",this,threadId_);
    if (t_loopInThisThread != nullptr) {
        LOG_FATAL("Anthor EventLoop %p exists in this thread %d \n",this,threadId_);
    }
    else {
        t_loopInThisThread = this;
    }
    /*设置wakeup channel感兴趣的事件 和 发生事件的回调*/
    {
        wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
        wakeupChannel_->enableReading(); /*每一个eventloop都将监听wakeup fd的读事件*/
    }
}
EventLoop::~EventLoop(){
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
    /*
    智能指针new出来的资源不需要自己释放
    */
}

/**
 * @brief  开启循环
 * 开启poller
 */
void EventLoop::loop(){
    looping_ = true;
    quit_ = false;
    LOG_INFO("EventLoop %p start looping\n",this);
    while (!quit_) {
        activeChannels_.clear();
        /*监听两类fd 一种是client,另外一种是wakeup_fd*/
        pollReturnTime_ = poller_->poll(kPollTimeMs,&activeChannels_);
        for(Channel* channel:activeChannels_) {
            /*poller监听哪些事件发生了事件，上报给eventloop
            *通知channel处理相应的事件
            */
            channel->handleEvent(pollReturnTime_);
        }
        /*
        *IO线程 接受新用户的连接 返回与客户端通信的fd(channel)
        *把channel分发给subloop 
        *main loop事先注册一个回调cb(需要subloop执行)
        */
        doPendingFunctors(); /*执行当前事件循环需要处理的回调操作*/
    }
    LOG_INFO("EventLoop %p stop looping\n",this);
    looping_ = false;
}
//退出事件循环 1.在自己的线程中则直接退出，不在自己的线程则调用loop的quit
void EventLoop::quit(){
    quit_ = true;
    /*????*/
    if(!isInLoopThread()) {
        wakeup();
    }
}

//在当前loop中执行cb
void EventLoop::runInLoop(Functor cb){
    /*在当前的线程中执行callback*/
    if(isInLoopThread()){
        cb();
    }
    else{
        queueInLoop(cb);
    }
}

//把cb放入队列中，唤醒loop所在线程，执行cb
void EventLoop::queueInLoop(Functor cb){
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    /*唤醒相应的需要执行操作的loop的线程
    *|| callingPendingFunctors_的意思是：当前loop正在执行回调，但是loop又有了新的回调
    */
    if( !isInLoopThread() || callingPendingFunctors_) {
        wakeup(); //唤醒loop所在线程
    }
}

//唤醒loop所在的线程
void EventLoop::wakeup(){
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one) {
        LOG_ERROR("EventLoop writes %lu instead of 8",n);
    }
}

// EventLoop的方法=》Poller的方法
void EventLoop::updateChannel(Channel *channel){
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel){
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel){
    return poller_->hasChannel(channel);
}

//判断EvenLoop对象是否在自己的线程里面

/**
 * @brief  唤醒线程后执行的回调
 */
void EventLoop::handleRead(){
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_,&one,sizeof one);
    if(n != sizeof one){
        LOG_ERROR("EventLoop:handleRead() reads %lu instead of 8",n);
    }
}
/**
 * @brief  被唤醒后需要执行的回调(subloop执行)
 */
void EventLoop::doPendingFunctors(){
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        swap(functors, pendingFunctors_);
    }
    for(const Functor& functor:functors) {
        functor();
    }
    callingPendingFunctors_ = false;
} 