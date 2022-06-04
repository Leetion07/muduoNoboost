/**
 * @File Name: Poller.h
 * @brief  IO复用的抽象类
 * @Author : leetion in hust email:leetion@hust.edu.cn
 * @Version : 1.0
 * @Creat Date : 2022-04-09
 */

#pragma once
#include "noncopyable.h"
#include "Timestamp.h"
#include <vector>
#include <unordered_map>
class Channel;
class EventLoop;
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel *>;
    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    //给所IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

    //判断参数channel是否在当前poller中
    bool hasChannel(Channel* channel) const;

    //EventLoop调用该方法可以获取默认的IO复用方法
    static Poller* newDefaultPoller(EventLoop* loop);
protected: 
    //map<key,value>,key对应sockfd,value对应fd所属通道 
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;
private:
    EventLoop *ownerLoop_;
};