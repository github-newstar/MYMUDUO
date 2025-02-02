#pragma once
#include "noncopyable.h"

class InetAddress;

class Socket : noncopyable
{
public:
    explicit Socket(int sockfd ): sockfd_(sockfd){}
    ~Socket();
    
    const int fd() const {return sockfd_;}
    void bindAddress(const InetAddress& localAddr);
    void listen();
    int accept(InetAddress *peerAddrr);
    
    void shutdownWrite();
    
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepALive(bool on);
private:
    const int sockfd_;
};