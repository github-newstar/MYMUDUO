#include "EventLoopThread.h"

EventLoopThread::EventLoopThread(
    const ThreadInitCallBack &cb ,
    const std::string &name)
    : loop_(nullptr)
      , exiting_(false)
      , thread_(std::bind(&EventLoopThread::threadFunc, this), name)
      , mutex_()
      , cond_()
      , callback_(cb)
     {
        
     }
EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if(loop_ != nullptr){
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop() {
    //启动底层新线程
    thread_.start();
    
    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [&]() { return loop_ != nullptr; });
        loop = loop_;
    }
    return loop;
}
void EventLoopThread::threadFunc() {
    //创建一个eventLoop,和上面的线程是一一对应的，one loop per thread
    EventLoop loop; 
    if(callback_){
        callback_(&loop);
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    
    loop.loop();
    
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}