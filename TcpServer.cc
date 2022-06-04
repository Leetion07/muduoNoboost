#include "TcpServer.h"
#include "Logger.h"
#include "EventLoop.h"
#include "TcpConnection.h"

#include <strings.h>
#include <functional>
using namespace std::placeholders;
EventLoop *checkLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d main loop is null\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &listenAddr,
                     const std::string &name,
                     Option option)
    : loop_(checkLoopNotNull(loop)), ipPort_(listenAddr.toIpPort()), name_(name), acceptor_(new Acceptor(loop_, listenAddr, option == kReusePort)), ThreadPool_(new EventLoopThreadPool(loop, name_)), connectionCallback_(), messageCallback_(), nextConnId_(1), started_(0)
{
    acceptor_->setNewConnectionCallback(std::bind(
        &TcpServer::newConnection, this, _1, _2));
}
TcpServer::~TcpServer()
{
    for (auto &item : connections_)
    {
        // 这个局部的shared_ptr智能指针对象，出右括号，可以自动释放new出来的TcpConnection对象资源了
        TcpConnectionPtr conn(item.second); 
        item.second.reset();

        // 销毁连接
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn)
        );
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    ThreadPool_->setThreadNum(numThreads);
}
void TcpServer::start()
{
    if (started_++ == 0) //防止tcp_server被开始多次
    {
        ThreadPool_->start();
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
        // loop_->loop();
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    EventLoop *ioloop = ThreadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;
    LOG_INFO("TcpServer::newConnection[%s] - new connection [%s] from %s \n",
        name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str()
    );
    /*通过sockfd获取其绑定的本机的ip地址的端口和信息*/
    sockaddr_in local;
    bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if(::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0){
        LOG_ERROR("socket::get local address\n");
    }
    InetAddress localAddr(local);

    /*根据连接成功的sockfd,创建TcpConnection对象*/
    TcpConnectionPtr conn(new TcpConnection(ioloop, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;
    /*下面的回调都是用户设置给TcpServer —> TcpConnection -> Channel -> Poller -> Channel调用回调*/
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    
    /*设置了如何关闭连接的回调*/
    conn->setCloseCallback(std::bind(
        &TcpServer::removeConnection,this,std::placeholders::_1
    ));

    ioloop->runInLoop(std::bind(
        &TcpConnection::connectEstablished,conn
    ));

}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(std::bind(
        &TcpServer::removeConnectionInLoop,this,conn
    ));
}
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n", 
        name_.c_str(), conn->name().c_str());

    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop(); 
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn)
    );
}
