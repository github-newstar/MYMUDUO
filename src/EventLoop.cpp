#include"EventLoop.h"
#include<sys/eventfd.h>
#include"Poller.h"
#include"Logger.h"
#include<unistd.h>
#include<fcntl.h>
#include"Channel.h"
#include<memory>

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

void EventLoop::loop(){
    looping_ = true;
    quit_ = false;
    
    LOG_INFO("EventLoop %p start looping \n", this);
    
    while(!quit_){
        activeChannels_.clear();
        // 监听两类fd 1 client_fd 2 wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for(auto *channel : activeChannels_){
            //Poller监听哪些channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        //执行当前eventLoop需要执行的回调操作
        /**
          * IO线程 mainLoop accept fd <= channel subLoop
          * mainLoop 事先注册一个cb（需要subLoop)执行  wakeup subloop then do cb
         */
        doPendingFunctors();
    }
    
    LOG_INFO("EventLoop %p stop looping", this);
    looping_ = false;
}
//退出事件循环 eventLoop在自己的线程中执行quit
void EventLoop::quit(){
    quit_ = true;
    
    //如果在其他线程中调用quit  在一个subLoop(worker)中，调用了mainLoop（IO）的quit
    if(!isInLoopThread()){ 
        wakeup();
    }
}

  //在loop中执行cb
void EventLoop::runInLoop(Functor cb){
    if(isInLoopThread()){
        cb();
    }else{
        queueInLoop(cb);
    }

}
//把cb放入队列，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb){
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    
    //唤醒相应的，需要执行上面回调操作的loop的线程
    if(!isInLoopThread() || callingPendingFuntionors_){
        wakeup();
    }
}
void EventLoop::handleRead(){
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if( n != sizeof one){
        LOG_ERROR("EventLoop: handleRead() reads %d bytes instead of 8", static_cast<int>(n));
    }
}
//用来唤醒loop所在的线程，向wake_fd写一个数据， wakeupChannel发生读事件，当前线程唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

// eventLoop的方法 =>   poller的方法
void EventLoop::updateChannel(Channel *channel) {
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel) {
    poller_->updateChannel(channel);
}
void EventLoop::hasChannel(Channel *channel) {
    poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFuntionors_ = true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors = pendingFunctors_;
    }
    for(const Functor &fun : functors){
        fun();
    }
    callingPendingFuntionors_ = false;
}