/**
 * @File Name: EpollPoller.h
 * @brief epollpoll IO复用的封装
 * @Author : leetion in hust email:leetion@hust.edu.cn
 * @Version : 1.0
 * @Creat Date : 2022-04-09
 *
 */
#include "Poller.h"
#include <vector>
#include <sys/epoll.h>
class Channel;

class EpollPoller : public Poller
{
public:
    EpollPoller(EventLoop* loop);
    ~EpollPoller();

    //重写IO接口
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    virtual void updateChannel(Channel *channel) override;
    virtual void removeChannel(Channel *channel) override;

private:
    static const int kInitEventListSize = 16;
    //填写活跃的连接
    void fillActiveChannels(int numEvents,ChannelList* activeChannels) const;
    //更新channel通道
    void update(int operation,Channel* channel);

    using EventList = std::vector<epoll_event>;
    int epollfd_;
    EventList events_;
};