#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop* loop)
    : ownerLoop_(loop) {}

Poller::~Poller() {}

bool Poller::hasChannel(Channel *channel) const {
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

Poller* Poller::newDefaultPoller(EventLoop *loop) {
    // TODO: 返回默认的IO复用实现
    return nullptr;
}

