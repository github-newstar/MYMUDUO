#include "Channel.h"
#include "Logger.h"
#include <memory>
#include<sys/epoll.h>


 const int Channel::kNoneEvent = 0;
 const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
 const int Channel::kWriteEvent = EPOLLOUT;

//EventLoop: ChannelList Poller
Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1)
    , tied_(false)
{
}
Channel::~Channel()
{
}


// TODO: 何时调用channel的tie方法
void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}
/**
 *  当改变channel表示的fd的events的事件后，update负责在poller里更改fd的对应的事件  epoll_ctl
 *  EventLoop --> ChannelList --> Poller
 */
void Channel::update(){
    // 通过channel所属的EventLoop，调用poller相应的方法，注册fd的events事件
    // TODO: loop_->updateChannel(this)
}

//在channel所属的eventLoop中，把当前的channel删除掉
void Channel::remove(){
    
}

void Channel::handleEvent(TimeStamp receiveTime) {
    std::shared_ptr<void> guard;
    if(tied_){
        guard = tie_.lock();
        if(guard){
            handleEventWithGuard(receiveTime);
        }
    }else{
        handleEventWithGuard(receiveTime);
    }
}

// 根据poller通知的channel发生的具体的事件，由channel负责具体的回调操作
void Channel::handleEventWithGuard(TimeStamp receiveTime) {
    
    LOG_INFO("channel handleEvent revents:%d\n", revents_);
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)){
        if(closeCallback_) closeCallback_();
    }
    if(events_ & EPOLLERR){
        if(errorCallback_) errorCallback_();
    }
    if(events_ & (EPOLLIN | EPOLLPRI)){
        if(readCallback_) readCallback_(receiveTime);
    }
    if(events_ & EPOLLOUT){
        if(writeCallback_) writeCallback_();
    }
}
