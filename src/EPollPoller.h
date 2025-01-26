#pragma once

#include "Poller.h"
#include<vector>
#include<sys/epoll.h>

class Channel;

class EPollPoller : public Poller
{
    public:
    EPollPoller(EventLoop *Loop);
    ~EPollPoller() override;
    
    //重写基类Poller抽象方法
    TimeStamp poll(int timeMs, ChannelList *activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel *channel) override;
    private:
    static const int kInitEventListSize = 16;
    
    //更新channel通道
    void fillActiveChanels(int numEvents, ChannelList *activeChannels) const;
    void update(int operation, Channel* channel);

    using EventList = std::vector<epoll_event>;
    int epollfd_;
    EventList events_;
};
