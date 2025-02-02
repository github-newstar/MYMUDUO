#pragma once

#include "noncopyable.h"
#include <memory>
#include <string>
#include <atomic>
#include "InetAddr.h"
#include "Callback.h"
#include "Buffer.h"

class Channel;
class EventLoop;
class Socket;

/**
 * Tcpserver通过Accpetor拿到新用户连接 通过accept函数拿到connfd
 * -> TcpConnection 设置回调 -> Channel -> Poller -> Channel的回调操作
 */
class TcpConnection : noncopyable , public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop,
                const std::string &name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAdder);
    ~TcpConnection();
    
    EventLoop* getLoop() const { return loop_;}
    const std::string&   name() const { return name_;}
    const InetAddress&  localAddress() const { return localAddr_;}
    const InetAddress&  peerAddress() const { return peerAddr_;}
    
    bool connected() const { return state_ == KConnected;}
    
    //发送数据
    void send(const std::string &buffer);
    void sendInLoop(const void* message, int len) ;
    
    //关闭连接
    void shutDown();
    void shutDownInLoop();
    void setConnectionCallback(const ConnectionCallback &cb) {
        conncectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback &cb) {
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
        writeCompleteCallback_ = cb;
    }
    void setCloseCallback(const CloseCallback &cb){
        closeCallback_ = cb;
    }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb,
                                  size_t highWaterMark) {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }
    
    void connectEstalished();
    void connectDestoryed();

private:
    enum State{ kDisconnected, kConnecting, KConnected, kDisconnecting};
    void setState(State state){ state_ = state;}
    
    void handleRead(TimeStamp reveiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    EventLoop *loop_;   //这里绝对不是mainLoop
    const std::string name_;
    std::atomic_int state_;
    bool reading_;
    
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    
    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback conncectionCallback_;    //有新连接时的回调
    MessageCallback messageCallback_;           //有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;//消息发送完成后的回调
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    
    size_t highWaterMark_;
    Buffer inputBuffer_;    //接收数据缓冲区
    Buffer outputBuffer_;   //发送数据缓冲区

};