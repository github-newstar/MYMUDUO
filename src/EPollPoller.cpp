#include "EPollPoller.h"
#include "Logger.h"
#include <cstring>
#include <error.h>
#include <unistd.h>
#include"Channel.h"
#include "TimeStamp.h"

//channel未添加到poller中
const int kNew = -1;    //channel的成员index_ = -1
//channel添加到了poller中
const int kAdded = 1;
//channel从index中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *Loop)
    : Poller(Loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize){
    if(epollfd_ < 0){
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}
EPollPoller::~EPollPoller(){
    ::close(epollfd_);
}

 TimeStamp EPollPoller::poll(int timeMs, ChannelList *activeChannels){
     // TODO  使用LOG_DEBUG
     LOG_INFO("func %s => fd total count:%d\n", __FUNCTION__, static_cast<int>(channels_.size()));
     
     int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeMs);
     int savceErrno = errno;
     
     TimeStamp now(TimeStamp::now());
     if(numEvents > 0){
        LOG_INFO("%d events happend \n", numEvents);
        fillActiveChanels(numEvents, activeChannels);
        if(numEvents == events_.size()){
            events_.resize(2 * events_.size());
        }
        else if(numEvents == 0){
            LOG_DEBUG("%s timeout! \n", __FUNCTION__);
        }else{
            if(savceErrno != EINTR){
                errno = savceErrno;
                LOG_ERROR("EPollPoller::poll() error");
            }
        }
     }
     return now;
     
 }
void EPollPoller::updateChannel(Channel *channel){
    const int index = channel->index();
    LOG_INFO("func = %s fd = %d events = %d index = %d", __FUNCTION__, channel->fd(), channel->events(), index);
    
    if(index == kNew || index == kDeleted){
        int fd = channel->fd();
        channels_[fd] = channel;
        
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }else{
        // int fd = channel->fd();
        if(channel->isNoneEvent()){
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }else{
            update(EPOLL_CTL_MOD, channel);
        }
    }

}
//从Poller中删除channel
void EPollPoller::removeChannel(Channel *channel){
    LOG_INFO("func %s\n", __FUNCTION__);
    int fd = channel->fd();
    channels_.erase(fd);
    
    int index = channel->index();
    if( index == kAdded){
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}
void EPollPoller::fillActiveChanels(int numEvents,
                                    ChannelList *activeChannels) const{
    for(int i = 0; i < numEvents; ++i){
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->emplace_back(channel);
    }
}
void EPollPoller::update(int operation, Channel *channel){
    epoll_event event;
    memset(&event, 0, sizeof event);
    int fd = channel->fd();
    event.events = channel->events();
    event.data.ptr = channel;
    if(epoll_ctl(epollfd_, operation, fd, &event) < 0){
        if(operation == EPOLL_CTL_DEL){
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }else{
            LOG_FATAL("epoll_ctl del error:%d\n", errno);
        }
    }
}