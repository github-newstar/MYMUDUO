#pragma once

#include "TimeStamp.h"
#include "noncopyable.h"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include "CurrentThread.h"
class Channel;
class Poller;

//事件循环类  包含两大模块 Channel Poller
class EventLoop : noncopyable {
public:
  using Functor = std::function<void()>;
  
  EventLoop();
  ~EventLoop();

  //开启事件循环
  void loop();
  //退出事件循环
  void quit();

  TimeStamp pollReturnTime() const { return pollReturnTime_; }
  
  //在loop中执行cb
  void runInLoop(Functor cb);
  //把cb放入队列，唤醒loop所在的线程，执行cb
  void queueInLoop(Functor cb);
  
  //唤醒loop 所在的线程
  void weakup();
  
  //eventLoop的方法 =>   poller的方法
  void updateChannel(Channel* channel);
  void removeChannel(Channel* channel);
  void hasChannel(Channel* channel);
  
  //判断event对象是否在自己的线程里面
  bool isInLoopThread() const {return threadId_ == CurrentThread::tid();}
    

private:
  //wake up
  void handleRead();
  //执行回调
  void doPendingFunctors();
  using ChannelList = std::vector<Channel *>;

  std::atomic_bool looping_; // CAS
  std::atomic_bool quit_;    //标识t退出loop循环
  const pid_t threadId_;     //记录当前线程tid
  TimeStamp pollReturnTime_; //记录poller返回发生事件的channels的时间点
  std::unique_ptr<Poller> poller_;

  int wakeupFd_; //当mainLoop获取一个新用户的channel,
                 //通过轮询算法选择一个subloop,通过该成员唤醒loop
  std::unique_ptr<Channel> wakeupChannel_;

  ChannelList activeChannels_;
  Channel *currentActiveChannel_;

  std::atomic_bool
      callingPendingFuntionors_; //标识当前的loop是否有需要执行的回调操作
  std::vector<Functor> pendingFunctors_; //存储loop所需要执行的所有回调操作
  std::mutex mutex_; //互斥锁，用于保护上面的vector容器的线程安全
};