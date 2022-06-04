#include "Buffer.h"
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>
/**
 * @brief  从fd上读取数据, 工作在LT模式
 * Buffer缓冲区是有大小的，却不知道TCP数据最终的大小
 * @param  fd:
 * @param  saveErrno:
 * @return ssize_t:
 */
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char extrabuf[65536] = {0}; //栈上的空间,64K
    iovec vec[2];
    const size_t writable = writableBytes(); //底层缓冲区剩余的可写空间大小
    vec[0].iov_base = beginWrite();
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable)
    {
        writerIndex_ += n;
    }
    else
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int* saveErrno){
    ssize_t n = ::write(fd, peek(), readableBytes());
    if(n < 0){
        *saveErrno = errno;
    }
    return n;
}