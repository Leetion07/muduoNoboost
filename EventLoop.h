/**
 * @File Name: EventLoop.h
 * @brief
 * @Author : leetion in hust email:leetion@hust.edu.cn
 * @Version : 1.0
 * @Creat Date : 2022-04-09
 *
 */

#pragma once
#include "noncopyable.h"
#include "Timestamp.h"
#include "Poller.h"
#include "CurrentThread.h"

#include <functional>
#include <vector>
#include <atomic>
#include <mutex>
#include <memory>

class Channel;
class Poller;
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    //开启事件循环
    void loop();
    //退出事件循环
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }
    //在当前loop中执行cb
    void runInLoop(Functor cb);
    //把cb放入队列中，唤醒loop所在线程，执行cb
    void queueInLoop(Functor cb);
    //唤醒loop所在的线程
    void wakeup();

    // EventLoop的方法=》Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    //判断EvenLoop对象是否在自己的线程里面
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); } 
private:
    void handleRead();          // wake up;
    void doPendingFunctors();   //执行回调

    using ChannelList = std::vector<Channel *>;

    std::atomic_bool looping_;  //原子操作,通过CAS实现·
    std::atomic_bool quit_;     //标识退出循环
    std::atomic_bool callingPendingFunctors_; //标识当前loop是否有需要执行的回调操作
    const pid_t threadId_;      //loop所在线程id,One loop per thread！

    Timestamp pollReturnTime_;  //poller返回事件的发生时间
    std::unique_ptr<Poller> poller_;

    int wakeupFd_;              //唤醒subreactor的fd,使用eventfd唤醒(一种线程通知机制man eventfd)
    std::unique_ptr<Channel> wakeupChannel_; //channel用于绑定fd和事件

    ChannelList activeChannels_;              //管理的所有channel
    Channel* currentActiveChannel_;           
    std::vector<Functor> pendingFunctors_;    //存储loop所需要执行的所有回调操作
    std::mutex mutex_;                        //互斥锁，用于保护vector的线程安全
};