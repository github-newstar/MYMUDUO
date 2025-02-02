#include "Acceptor.h"
#include "Logger.h"
#include "InetAddr.h"
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
static int createNoblocking(){
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                          IPPROTO_TCP);
    if(sockfd < 0){
        LOG_FATAL("%s:%s:%d listen socket create error:%d \n", __FILE__,
                  __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}
Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr,
                   bool reusePort)
    : loop_(loop), acceptSocket_(createNoblocking()),
      acceptChannel_(loop, acceptSocket_.fd()), listenning_(false) {
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr);
    // TcpServer::start Accpetor.listen() 有新用户连接，要执行一个回调(connfd =>
    // channel => subloop) baseLoop => acceptChannel_(listenfd) =>
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}
Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}
void Acceptor::listen() {
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}
//liseten有事件发生，就是有新用户连接
void Acceptor::handleRead() {
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0){
        if(newConnnectionCallback_){
            newConnnectionCallback_(connfd, peerAddr);
        }else{
            ::close(connfd);
        }
    }else{
        LOG_ERROR("%s:%s:%d accept err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        if(errno == EMFILE){
        LOG_ERROR("%s:%s:%d socket reached limit! \n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}