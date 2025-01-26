#include"EventLoop.h"
#include<sys/eventfd.h>
#include"Poller.h"
#include"Logger.h"
#include<unistd.h>
#include<fcntl.h>
#include"Channel.h"

//防止一个线程创建多个EventLoop 
__thread EventLoop *t_loopInThisThread = nullptr;

//定义默认的Poller IO复用接口超时的时间
const int kPollTimeMs = 1000;

//创建wakeupfd,用来唤醒subReactor处理新来的channel
int createEventfd(){
    int evtfd = ::eventfd(0, EFD_NONBLOCK |  EFD_CLOEXEC);
    if(evtfd < 0){
        LOG_FATAL("eventfd error%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
     : looping_(false)
       , quit_(false)
       , threadId_(CurrentThread::tid())
       , poller_(Poller::newDefaultPoller(this))
       , wakeupFd_(createEventfd())
       , wakeupChannel_( new Channel(this, wakeupFd_))
       , currentActiveChannel_(nullptr)
       , callingPendingFuntionors_(false)

{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if(t_loopInThisThread){
        LOG_FATAL("Another EventLoop  %p in this thread %d \n", t_loopInThisThread, threadId_);
    }else{
        t_loopInThisThread = this;
    }
    
    //设置wakeup fd的事件类型以及相应的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个eventLoop都将监听wakeupchannel的epollin读事件
    wakeupChannel_->enableReading();
}
EventLoop::~EventLoop(){
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::handleRead(){
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if( n != sizeof one){
        LOG_ERROR("EventLoop: handleRead() reads %d bytes instead of 8", static_cast<int>(n));
    }
}