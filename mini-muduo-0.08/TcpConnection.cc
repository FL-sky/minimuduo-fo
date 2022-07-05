// author voidccc

#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Define.h"
#include "IMuduoUser.h"

#include <string.h> //for bzero
#include <iostream>
#include <unistd.h>
using namespace std;

TcpConnection::TcpConnection(int sockfd, EventLoop *pLoop)
    : _sockfd(sockfd), _pLoop(pLoop), _pUser(NULL)
{
    _pChannel = new Channel(_pLoop, _sockfd); // Memory Leak !!!
    _pChannel->setCallback(this);
    _pChannel->enableReading();
}

TcpConnection::~TcpConnection()
{
}

void TcpConnection::handleRead()
{
    int sockfd = _pChannel->getSockfd();
    int readlength;
    char line[MAX_LINE];
    if (sockfd < 0)
    {
        cout << "EPOLLIN sockfd < 0 error " << endl;
        return;
    }
    bzero(line, MAX_LINE);
    if ((readlength = read(sockfd, line, MAX_LINE)) < 0)
    {
        if (errno == ECONNRESET)
        {
            cout << "ECONNREST closed socket fd:" << sockfd << endl;
            close(sockfd);
        }
    }
    else if (readlength == 0)
    {
        cout << "read 0 closed socket fd:" << sockfd << endl;
        close(sockfd);
    }
    else
    {
        string linestr(line, readlength);
        _inBuf.append(linestr);
        _pUser->onMessage(this, &_inBuf);
    }
}

// 下面的条目1 ～ 条目5讲解onWireteComplate的实现细节，条目6讲解Buffer。

// 1 先来看看IMuduoUser的onWriteComplate回调，首先要明确的是onWriteComplate回调什么时候会被调用，那就是用户递交数据给网络库，
// 当网络库把这些信息全部传送给操作系统后(通过::write()调用)，用户对象的onWriteComplate会被回调到。
// muduo使用的是level triger的Epoll，所以应该在下面两个位置调用onWriteComplate。
// 注意这两处不是直接通过_pUser->onWriteComplate(this);的方式调用的，而是使用了EventLoop::queueLoop方法来异步调用。
// 下条说明会对queueLoop方法做详细介绍。在原始muduo里关于这个回调的讲解可参见<<LInux多线程服务端编程>>P322页。

void TcpConnection::send(const string &message)
{
    int n = 0;
    if (_outBuf.readableBytes() == 0)
    {
        n = ::write(_sockfd, message.c_str(), message.size());
        if (n < 0)
            cout << "write error" << endl;
        if (n == static_cast<int>(message.size()))
            _pLoop->queueLoop(this); // invoke onWriteComplate
        // 位置1：第一次调用size_t n = ::write(...len...) 后，n和len相等的时候，
        // 这表明一次系统调用已经将数据全部发送完了，需要通知用户了。调用点位于TcpConnection.cc pLoop->queueLoop(this);
    }

    if (n < static_cast<int>(message.size()))
    {
        _outBuf.append(message.substr(n, message.size()));
        if (_pChannel->isWriting())
        {
            _pChannel->enableWriting(); // add EPOLLOUT
        }
    }
}

// 位置2： 当某次调用size_t n = ::write(...len...) 后，n < len，
// 表明操作系统无法全部接收本次递交给他的数据。
// 当操作系统的发送缓冲区有更多可用空间时，通过让epoll_wait返回来通知我们发生了EPOLLOUT事件，
// 这时网络库会把自己保存缓冲区数据继续送给操作系统，
// 如果全部数据操作系统都接收完毕，这时也需要通知用户了。
// 调用点位于TcpConnection.cc [_pChannel->disableWriting();] _pLoop->queueLoop(this);
void TcpConnection::handleWrite()
{
    int sockfd = _pChannel->getSockfd();
    if (_pChannel->isWriting())
    {
        int n = ::write(sockfd, _outBuf.peek(), _outBuf.readableBytes());
        if (n > 0)
        {
            cout << "write " << n << " bytes data again" << endl;
            _outBuf.retrieve(n);
            if (_outBuf.readableBytes() == 0)
            {
                _pChannel->disableWriting(); // remove EPOLLOUT
                _pLoop->queueLoop(this);     // invoke onWriteComplate
            }
        }
    }
}
// 在这两处设置好回调后，即可保证用户得到准确的通知。
void TcpConnection::connectEstablished()
{
    if (_pUser)
        _pUser->onConnection(this);
}

void TcpConnection::setUser(IMuduoUser *user)
{
    _pUser = user;
}

void TcpConnection::run()
{
    _pUser->onWriteComplate(this);
}
