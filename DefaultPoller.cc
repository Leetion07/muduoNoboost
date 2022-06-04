#include "Poller.h"
#include "EpollPoller.h"
#include <stdlib.h>

Poller* Poller::newDefaultPoller(EventLoop* loop){
    if(::getenv("MUDUO_USE_POLL")){
        //to add...
        return nullptr; 
    }
    else{
        return new EpollPoller(loop);
    }
}