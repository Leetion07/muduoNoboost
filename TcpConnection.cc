#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Buffer.h"
#include "EventLoop.h"

#include <string>
#include <functional>
#include <errno.h>
#include <memory>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
using namespace std::placeholders;
static EventLoop *checkLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnction loop is null\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &name,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(checkLoopNotNull(loop)), name_(name),
      state_(kConnecting), reading_(true),
      socket_(new Socket(sockfd)), channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr), peerAddr_(peerAddr_), highWaterMark_(64 * 1024 * 1024)
{
    /*在channel上注册相应的回调函数*/
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}
TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d\n", name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::send(const std::string &buf)
{
    LOG_DEBUG("TcpConnection::send is called,state is connected?%d\n",state_ == kConneced);
    if (state_ == kConneced)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            LOG_DEBUG("TcpConnection::send case2 is called\n");
            loop_->runInLoop(std::bind(
                &TcpConnection::sendInLoop,
                this,
                buf.c_str(),
                buf.size()));
        }
    }
}
void TcpConnection::shutdown()
{
    if (state_ == kConneced)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(
            &TcpConnection::shutdownInLoop, this));
    }
}

/*连接建立*/
void TcpConnection::connectEstablished()
{
    setState(kConneced);
    channel_->tie(shared_from_this());
    channel_->enableReading(); //向poller注册读事件
    /*新连接建立，执行回调*/
    connectionCallback_(shared_from_this());
}

/*连接断开*/
void TcpConnection::connectDestroyed()
{
    if (state_ = kConneced)
    {
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &saveErrno);
    if (n > 0)
    {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}
void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int saveErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &saveErrno);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down, no moreWriting \n", channel_->fd());
    }
}
void TcpConnection::handleClose()
{
    LOG_INFO("fd=%d state=%d\n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();
    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);
    closeCallback_(connPtr);
}
void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d", name_.c_str(), err);
}

/**
 * @brief  发送数据，需要写数据缓冲
 * @param  message:
 * @param  len:
 */
void TcpConnection::sendInLoop(const void *message, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing!\n");
        return;
    }
    /*表示channel第一次开始写数据，而且缓冲区没有待发送数据*/
    LOG_DEBUG("channel_->iswriting:%d,readableBytes()=%d",(int)channel_->isWriting(),(int)outputBuffer_.readableBytes());
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        LOG_DEBUG("TcpConnection::send in loop is tring!\n");
        
        nwrote = ::write(channel_->fd(), message, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            /*一次性数据发送完成，不用给channel设置epollout事件*/
            if (remaining == 0 && writeCompleteCallback_)
            {
                loop_->queueInLoop(std::bind(
                    writeCompleteCallback_, shared_from_this()));
            }
            else
            {
                nwrote = 0;
                if (errno != EWOULDBLOCK)
                {
                    LOG_ERROR("TcpConnection::SendInloop\n");
                    if (errno == EPIPE || errno == ECONNRESET)
                    {
                        faultError = true;
                    }
                }
            }
        }
    }

    /*说明当前这一次write,并没有全部发送出去，需要保存到缓冲区中，给channel注册EpollOut事件
     *Poller发现tcp的发送缓冲区有空间，则会通知sock-channel,调用writeCallback_
     *最终调用TcpConnection::handWrite方法，把发送缓冲区的数据全部发送完成
     */
    if (!faultError && remaining > 0)
    {
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append((char *)message + nwrote, remaining);
        if (!channel_->isWriting())
        {
            /*这里一定要注册channel的写事件，否则poller不会给channel通知epollout*/
            channel_->enableWriting();
        }
    }
}
void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting())
    {
        /*说明数据已经全部发送完成*/
        socket_->shutdownWrite();
    }
}