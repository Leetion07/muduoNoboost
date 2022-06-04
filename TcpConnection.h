#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"
#include <functional>
#include <memory>
#include <string>
#include <atomic>
class Channel;
class EventLoop;
class Socket;

/**
 * @brief  
 * TcpServer => Acceptor =>新用户连接 =>accept拿到connfd
 * =>TcpConnection设置回调  => Channel =>Poller => Channel上面的回调
 */
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, 
                const std::string& name, 
                int sockfd, 
                const InetAddress& localAddr, 
                const InetAddress& peerAddr);
    ~TcpConnection();
    EventLoop* getLoop() const {return loop_;}
    const std::string& name() const {return name_;}
    const InetAddress& localAddr() const {return localAddr_;}
    const InetAddress& peerAddr() const {return peerAddr_;}

    bool conneted() const {return state_ == kConneced;}

    void send(const std::string& buf);
    void shutdown();

    void setConnectionCallback(ConnectionCallback& cb){
        connectionCallback_ = cb;
    }
    void setMessageCallback(MessageCallback& cb){
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(WriteCompleteCallback& cb){
        writeCompleteCallback_ = cb;
    }
    void setHighWaterMarkCallback(HighWaterMarkCallback& cb,size_t highWaterMark){
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }
    void setCloseCallback(const CloseCallback& cb){
        closeCallback_ = cb;
    }

    void connectEstablished();
    void connectDestroyed();
private:
    enum StateE
    {
        kDisconnected,
        kConnecting,
        kConneced,
        kDisconnecting,
    };
    void setState(StateE state) { state_ = state; }

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();

    EventLoop *loop_; // subloop
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;       //有新连接时的回调
    MessageCallback messageCallback_;             //有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; //消息发送完成后的回调

    CloseCallback closeCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;

    size_t highWaterMark_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;
};