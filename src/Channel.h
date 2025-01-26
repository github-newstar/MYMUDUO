#pragma once
#include "TimeStamp.h"
#include"noncopyable.h"
#include<functional>
#include<memory>

class EventLoop;
class TimeStamp;
/**
 * Channel理解为通道，封装了sockfd和其感兴趣的event，如EPOLLIN、EPOLLOUT事件
 * 还绑定了poller返回的具体事件
 */
#include "noncopyable.h"
class Channel : noncopyable
{
    public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(TimeStamp)>;
    Channel(EventLoop *Loop, int fd);
    ~Channel();

    void handleEvent(TimeStamp receiveTime); // fd得到poller通知后，处理事件

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) {
        readCallback_ = std::move(cb);
    }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    EventLoop *ownerLoop() { return loop_; }
    void remove();

    //防止channel被remove掉后，channel还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_;}
    int events() const {return events_;}
    void set_revents(int revt) {revents_ = revt;}
    bool isNoneEvents() const {return events_ == kNoneEvent;}
    
    //设置fd相应的事件
    void enableReading(){ events_ |= kReadEvent; update();}
    void disableReading(){ events_ &= ~kReadEvent; update();}
    void enableWritting(){ events_ |= kWriteEvent; update();}
    void disableWritting(){ events_ &= ~kWriteEvent; update();}
    void disableAll(){ events_ = kNoneEvent; update();}
    
    //返回fd当前的状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isReading() const { return events_ & kReadEvent; }
    bool isWirtting() const { return events_ & kWriteEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    private:
    void update();
    void handleEventWithGuard(TimeStamp reveiveTime);
    
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;
    
    EventLoop *loop_;   //事件循环
    const int fd_;      //fd, poller监听的对象
    int events_;        //注册fd感兴趣的事件
    int revents_;       //
    int index_;
    
    std::weak_ptr<void> tie_;//管理channel关联对象的生命周期
    bool tied_;
    
    //因为channel通道能够获知fd最终发生的事件revents,所以它负责调用最后的具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback   writeCallback_;
    EventCallback   closeCallback_;
    EventCallback   errorCallback_;
};