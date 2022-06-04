#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop *loop):ownerLoop_(loop)
{
}
//判断参数channel是否在当前poller中
bool Poller::hasChannel(Channel *channel) const{
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}