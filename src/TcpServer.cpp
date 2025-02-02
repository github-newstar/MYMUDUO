#include"TcpServer.h"
#include "Logger.h"
#include <sys/socket.h>
#include <sys/types.h> 
#include <netinet/tcp.h>
#include <unistd.h>
#include <string.h>
#include "TcpConnection.h"

EventLoop * CheckloopNotNull(EventLoop *loop){
    if( loop == nullptr){
        LOG_FATAL("%s:%s:%d mainLoop is null !\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr,
                     const std::string &nameArg, Option option)
    : loop_(CheckloopNotNull(loop)), ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReuserPort))
   , threadPool_(new EventLoopThreadPool(loop, name_))
   , conncectionCallback_()
   , messageCallback_ ()
   , nextConnId_(1)
{
    // 当有新用户连接时，执行TcpServer::newConnectino回调
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,
                                                  this, std::placeholders::_1,
                                                  std::placeholders::_2));
}
TcpServer::~TcpServer(){
    for(auto &item : connections_){
        TcpConnectionPtr conn(item.second); //这个局部的shared_ptr对象，出括号可以自动释放new出来的TcpConnection对象资源
        item.second.reset();

        //销毁链接
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestoryed, conn));
    }
}

//有一个新的客户端连接 accpetor会执行这个回调
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
    //轮询算法 选择一个subLoop来管理channel
    EventLoop* ioLoop = threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof buf, ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;
    
    LOG_INFO("TcpServer::newConnection[%s] - new connection [%s] from %s \n",
        name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());
    //通过sockfd获取本机的ip和port信息
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if(::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0){
        LOG_ERROR("socket::getLoacal");
    }
    InetAddress localAddr(local);
    
    //根据连接成功的sockfd 创建TcpConnetion对象
    TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                        connName,
                                        sockfd,
                                        localAddr,
                                        peerAddr));
    connections_[connName] = conn;
    //下面的回调都是用户设置给TcpServer => TcpConnection => Channel => Poller => notify channel调用回调
    conn->setConnectionCallback(conncectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    //设置了如何关闭连接的回调
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstalished, conn));
}

void TcpServer::setThreadNum(int numsTreads){
    threadPool_->setThreadNum(numsTreads);
}

void TcpServer::start(){
    //防止一个tcpServer被多次启动
    if(started_++ == 0){
        threadPool_->start(threadInitCallback_); //启动底层线程池
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn) {
    LOG_INFO("TcpServer::removeConnectionInLoop[%s] - connection %s \n",
             name_.c_str(), conn->name().c_str());
    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestoryed, conn));
}