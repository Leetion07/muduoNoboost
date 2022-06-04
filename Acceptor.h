#pragma once
#include "noncopyable.h"
#include <functional>
#include "Socket.h"
#include "Channel.h"
class EventLoop;
class InetAddress;
class Acceptor:noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd,const InetAddress&)>;
    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb){
        newConnectionCallback_ = std::move(cb);
    }

    bool listenning() const {return listenning_;}
    void listen();
private:
    void handleRead();
    EventLoop* loop_; //mainloop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};