#pragma once

#include "EPollPoller.h"
#include "Logger.h"
#include <error.h>
#include <unistd.h>

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

}
void EPollPoller::updateChannel(Channel *channel){

}
void EPollPoller::removeChannel(Channel *channel){

}
void EPollPoller::fillActiveChanels(int numEvents,
                                    ChannelList *activeChannels) const{

                                    }
void EPollPoller::update(int operation, Channel *channel){
    const int index = channel->index();

}