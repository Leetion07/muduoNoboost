#pragma once

#include "noncopyable.h"
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <atomic>
/**
 * @brief  用户使用muduo库编写服务器程序
 */
class TcpServer:noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    enum Option{
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& name, Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback& cb) {threadInitCallback_ = cb;}
    void setConnectionCallback(const ConnectionCallback& cb) {connectionCallback_ = cb;}
    void setMessageCallback(const MessageCallback& cb) {messageCallback_ = cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {writeCompleteCallback_ = cb;}

    void setThreadNum(int numThreads); //设置subloop的个数
    void start(); //开启服务器监听
private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::unordered_map<std::string,TcpConnectionPtr>;
    EventLoop* loop_; //base loop,任务是监听新连接事件
    const std::string ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_; 
    std::shared_ptr<EventLoopThreadPool> ThreadPool_;   

    ConnectionCallback connectionCallback_;         //有新连接时的回调
    MessageCallback messageCallback_;               //有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;   //消息发送完成后的回调
    ThreadInitCallback threadInitCallback_;         //loop线程初始化的回调

    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_; //保存所有的连接

};