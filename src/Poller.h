#pragma  once
#include "EventLoop.h"
#include "TimeStamp.h"
#include "noncopyable.h"
#include <unordered_map>
#include <vector>

class Channel;
class EventLoop;
//muduo库多路事件分发器的核心IO复用模块
class Poller : noncopyable
{
    public:
    using ChannelList = std::vector<Channel *>;

    Poller(EventLoop *eventLoop);
    virtual ~Poller();

    // 给s所有IO复用保留统一的接口
    virtual TimeStamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    // 判断参数channel是够在当前poller当中
    bool hasChannel(Channel *channel) const;

    //EventLoop可以通过该接口获取默认的IO复用实现
    static Poller* newDefaultPoller(EventLoop *Loop);

    protected:
    //int 代表的是fd
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;

    private:
    EventLoop *ownerLoop_;
};