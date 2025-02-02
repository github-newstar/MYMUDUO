#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Socket.h"
#include "errno.h"
// #include <unistd.h>

static EventLoop *CheckLoopNotNull(EventLoop *loop) {
    if (loop == nullptr) {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __FUNCTION__,
                  __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &name,
                             int sockfd, const InetAddress &localAddr,
                             const InetAddress &peerAdder)
    : loop_(CheckLoopNotNull(loop)), name_(name), state_(kConnecting),
      reading_(true), socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)), localAddr_(localAddr),
      peerAddr_(peerAdder), highWaterMark_(64 * 1024 * 1024) {
    // 给下面的channel设置对应的回调函数，
    // poller给channel通知感兴趣的事件发生了， channel会回调相应的操作函数
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    LOG_INFO("TcpConnection::ctor[%s] at fd=%d \n", name_.c_str(), sockfd);
    socket_->setKeepALive(true);
}
TcpConnection::~TcpConnection() {
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d  state=%d \n", name_.c_str(),
             channel_->fd(), (int)state_);
}

void TcpConnection::send(const std::string &buffer) {
    if(state_ == KConnected){
        if(loop_->isInLoopThread()){
            sendInLoop(buffer.c_str(), buffer.size());
        }else{
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop,
                            this,
                            buffer.c_str(),
                            buffer.size()));
        }
    }
}

/**
 * 发送数据  应用发得快 内核写的慢,需要把发送数据写入缓冲区，而且设置了水位回调
 */
void TcpConnection::sendInLoop(const void * message, int len){
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    
    //之前调用或connection的shutdown 不能再发送
    if(state_ == kDisconnected){
        LOG_ERROR("disconnected, give up writting ");
        return;
    }
    
    //表示channel_第一次写数据 二桥缓冲区没有要发送的数据
    if(!channel_->isWirtting() && outputBuffer_.readableBytes() == 0){
        nwrote = ::write(channel_->fd(),message, len);
        if(nwrote >= 0){
            remaining = len -nwrote;
            //既然在这里数据一次性发送完成，就不用给channel给设置epollout 事件
            if(remaining == 0 && writeCompleteCallback_){
                loop_->runInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }else{ //nwrote < 0
            nwrote = 0;
            if(errno != EWOULDBLOCK){
                LOG_ERROR("TcpConnection::sendInLoop");
                if(errno == EPIPE || errno == ECONNRESET){
                    faultError = true;
                }
            }
        }
    }
    /**
     * 说明这一次的write,并没有把数据全部发送出去，剩余的数据需要保存到缓冲区里，然后给channel
     * 注册epollout事件，poller发现tcp的发送缓冲区有空间，会通知相应的sock-channel,调用handlewrite回调方法
     * 也就是调用TcpConnection::handleWrite,把缓冲区的内容全部发送完成
     */
    if(!faultError && remaining > 0){
        //目前发送缓冲区剩余的待发送数据
        size_t oldLen = outputBuffer_.readableBytes();
        if(oldLen + remaining >= highWaterMark_
            && oldLen < highWaterMark_
            && highWaterMarkCallback_){
                loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
            }
        outputBuffer_.append((char*)message + nwrote, remaining);
        if(!channel_->isWirtting()){
            //这里一定要注册channel的写事件，否则无法驱动poller的epollout
            channel_->enableReading();
        }
    }
}

void TcpConnection::handleRead(TimeStamp reveiveTime) {
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &saveErrno);
    if (n > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_, reveiveTime);
    } else if (n == 0) {
        handleClose();
    } else {
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handleError");
        handleError();
    }
}
void TcpConnection::handleWrite() {
    if (channel_->isWirtting()) {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {
                channel_->disableWritting();
                if (writeCompleteCallback_) {
                    // 唤醒loop对应的thread线程的回调
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting) {
                    shutDownInLoop();
                }
            }
        } else {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    } else {
        LOG_ERROR("TcpConnection fd=%d is down , no more writting \n",
                  channel_->fd());
    }
}
void TcpConnection::handleClose() {
    LOG_INFO("fd=%d state=%d \n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    conncectionCallback_(connPtr); // 执行连接关闭的回调
    closeCallback_(connPtr);       // 关闭回调
}
void TcpConnection::handleError() {
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) <
        0) {
        err = errno;
    } else {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n",
              name_.c_str(), err);
}
//创建连接
    void TcpConnection::connectEstalished(){
        setState(KConnected);
        channel_->tie(shared_from_this());
        channel_->enableReading();  //向channel注册epollin事件
        
        conncectionCallback_(shared_from_this());
    }
//销毁连接
    void TcpConnection::connectDestoryed(){
        if(state_ == KConnected){
            setState(kDisconnected);
            channel_->disableAll(); //把channel感兴趣的事件,从poller中del掉
            conncectionCallback_(shared_from_this());
        }
        channel_->remove(); //把channel从poller中删除掉
    }

void TcpConnection::shutDown(){
    if( state_== KConnected){
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutDownInLoop, this));
    };
}
void TcpConnection::shutDownInLoop(){
    if(channel_->isWirtting() == false){
        socket_->shutdownWrite();
    }
}