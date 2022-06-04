#include "mymuduo/TcpServer.h"
#include "mymuduo/Logger.h"
#include <string>

class EchoServer{
public:
    EchoServer(EventLoop* loop,
    const InetAddress& addr,
    const std::string& name)
    :server_(loop,addr,name)
    ,loop_(loop)
    {
        server_.setConnectionCallback(std::bind(
            &EchoServer::onConnection,this,std::placeholders::_1
        ));
        server_.setMessageCallback(std::bind(
            &EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3
        ));
        server_.setThreadNum(3);
    }

    void start(){
        server_.start();
    }
private:
    //连接建立或者断开
    void onConnection(const TcpConnectionPtr &conn){
        if(conn->conneted()){
            LOG_INFO("new connection %s up!\n",conn->peerAddr().toIpPort().c_str());
        }
        else{
            LOG_INFO("new connection %s down!\n",conn->peerAddr().toIpPort().c_str());
        }
    }

    void onMessage(const TcpConnectionPtr& conn,
                Buffer* buf,
                Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        LOG_INFO("receive %s\n",msg.c_str());
        conn->send("i am 1379 listenner! i receiver your msg:" + msg);
        conn->send("No reply!\nNo reply!\nNo reply!\n");
        conn->shutdown();
    }

    
    EventLoop* loop_;
    TcpServer server_;
};

int main(){
    EventLoop loop;
    InetAddress addr(8000);
    EchoServer server(&loop, addr, "EchoSerer-01");
    server.start();
    loop.loop(); //main loop的底层poller启动
    return 0;
}