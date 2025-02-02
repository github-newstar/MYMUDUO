#pragma once
#include "noncopyable.h"
#include <vector>
#include <string>


//网络库底层缓冲区定义
class Buffer : noncopyable
{
public:
    using size_t = std::size_t;
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;
    
    explicit Buffer(size_t initialSize = kInitialSize)
            : buffer_(kCheapPrepend + initialSize)
              , readerIndex_(kCheapPrepend)
              , writeIndex_(kCheapPrepend)
              {}
    
    size_t readableBytes() const{
        return writeIndex_ - readerIndex_;
    }
    size_t writeableBytes() const{
        return buffer_.size() - writeIndex_;
    }
    size_t prependableBytes() const {
        return readerIndex_;
    }

    //返回缓冲区中刻度数据的起始地址
    const char* peek() const {
        return begin() + readerIndex_;
    }
    
    //onMessage string <- Buffer
    void retrieve(size_t len){
        if(len < readableBytes()){
            //应用只读取了可读缓冲区的一部分
            //还有writeIndex-readerIndex的部分没有读取
            readerIndex_ += len;
        }else{
            retrieveAll();
        }
    }
    
    void retrieveAll(){
        readerIndex_ = writeIndex_ = kCheapPrepend;
    }
    
    //把onMessage函数上报的buffer数据，转换成string类的数据返回
    std::string retrieveAllAsString(){
        return retrieveAsString(readableBytes());
    }
    
    std::string retrieveAsString(size_t len){
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }
    
    void ensureWriteAbleBytes(size_t len){
        if(writeableBytes() < len){
            makeSpace(len);
        }
    }
    
    //把[data, data+len]上的数据添加到
    void append(const char* data, size_t len){
        ensureWriteAbleBytes(len);
        std::copy(data, data + len, beginWirte());
        writeIndex_ += len;
    }
    
    //从fd上兑取数据
    ssize_t readFd(int fd, int* saveErrno);
    ssize_t writeFd(int fd, int* saveErrno);
private:
    char * begin(){
        return &*buffer_.begin();
    }
    char *beginWirte(){
        return begin() + writeIndex_;
    }
    const char *beginWirte() const {
        return begin() + writeIndex_;
    }
    //扩容代码
    void makeSpace(size_t len){
        if(prependableBytes() + writeableBytes() < len + kCheapPrepend){
            buffer_.resize(writeIndex_ + len);
        }else{
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                      begin() + writeIndex_,
                      begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writeIndex_ = readerIndex_ + readable;
        }
    }
    const char * begin() const {
        return &*buffer_.begin();
    }
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writeIndex_;
    
};