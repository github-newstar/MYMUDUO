#include "Buffer.h"
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

// 从fd上兑取数据 Poller 工作在LT模式
// Buffer缓冲区有大小 从fd上读数据的时候 不知道tcp数据最终的大小
ssize_t Buffer::readFd(int fd, int *saveErrno) {
    //栈上空间64k
    char extraBuf[65536] = {0};
    struct iovec vec[2];
    const size_t writable = writeableBytes();   //这是buffer底层剩余的大小
    vec[0].iov_base = begin() + writeIndex_;
    vec[0].iov_len = writable;
    
    vec[1].iov_base = extraBuf;
    vec[1].iov_len = sizeof extraBuf;
    
    const int iovcnt = (writable < sizeof extraBuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if( n < 0){
        *saveErrno = errno;
    }else if(n <= (const ssize_t)writable){
        writeIndex_ += n;
    }else{
        writeIndex_ = buffer_.size();
        append(extraBuf, n - writable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno) {
    ssize_t n = ::write(fd, peek(), readableBytes());
    if(n < 0){
        *saveErrno =errno;;
    }
    return n;
}
