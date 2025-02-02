#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"


class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;
    Acceptor(EventLoop* loop, const InetAddress &listenAddr, bool reusePort);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &cb) {
        newConnnectionCallback_ = cb;
    }

    bool listenning() const { return listenning_;}
    void listen();

private:
    void handleRead();
    EventLoop *loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnnectionCallback_;
    bool listenning_;
};