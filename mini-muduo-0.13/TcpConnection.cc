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
#include "Task.h"

#include <string.h> //for bzero
#include <iostream>
#include <unistd.h>
using namespace std;

TcpConnection::TcpConnection(int sockfd, EventLoop *pLoop)
    : _sockfd(sockfd), _pLoop(pLoop), _pUser(NULL)
{
    _pSocketChannel = new Channel(_pLoop, _sockfd); // Memory Leak !!!
    _pSocketChannel->setCallback(this);
    _pSocketChannel->enableReading();
}

TcpConnection::~TcpConnection()
{
}

void TcpConnection::handleRead()
{
    int sockfd = _pSocketChannel->getfd();
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

void TcpConnection::handleWrite()
{
    int sockfd = _pSocketChannel->getfd();
    if (_pSocketChannel->isWriting())
    {
        int n = ::write(sockfd, _outBuf.peek(), _outBuf.readableBytes());
        if (n > 0)
        {
            cout << "write " << n << " bytes data again" << endl;
            _outBuf.retrieve(n);
            if (_outBuf.readableBytes() == 0)
            {
                _pSocketChannel->disableWriting(); // remove EPOLLOUT
                Task task(this);
                _pLoop->queueInLoop(task); // invoke onWriteComplate
            }
        }
    }
}

void TcpConnection::send(const string &message)
{
    if (_pLoop->isInLoopThread())
    {
        sendInLoop(message);
    }
    else
    {
        Task task(this, message, this);
        _pLoop->runInLoop(task);
    }
}

// 2 TcpConnection

// 添加了一个sendInLoop方法，把原来send方法里的实现移动到了sendInLoop方法里，而send方法本身变成了一个外部接口的包装。
// 根据调用send方法所在线程的不同，采取不同的策略，
// 如果调用TcpConnection::send的线程刚好是IO线程，则立刻使用write将数据送出（当然是缓冲区为空的时候）。
// 如果调用TcpConnection::send的线程是work线程(也就是后台处理线程)则只将要发送的信息通过Task对象丢到EventLoop的异步队列中，然后立刻返回。
// EventLoop随后会在IO线程回调到TcpConnetion::sendInLoop方法，这样做的目的是保证网络IO相关操作只在IO线程进行。

void TcpConnection::sendInLoop(const string &message)
{
    int n = 0;
    if (_outBuf.readableBytes() == 0)
    {
        n = ::write(_sockfd, message.c_str(), message.size());
        if (n < 0)
            cout << "write error" << endl;
        if (n == static_cast<int>(message.size()))
        {
            Task task(this);
            _pLoop->queueInLoop(task); // invoke onWriteComplate
        }
    }

    if (n < static_cast<int>(message.size()))
    {
        _outBuf.append(message.substr(n, message.size()));
        if (!_pSocketChannel->isWriting())
        {
            _pSocketChannel->enableWriting(); // add EPOLLOUT
        }
    }
}

void TcpConnection::connectEstablished()
{
    if (_pUser)
        _pUser->onConnection(this);
}

void TcpConnection::setUser(IMuduoUser *user)
{
    _pUser = user;
}

void TcpConnection::run0()
{
    _pUser->onWriteComplate(this);
}

void TcpConnection::run2(const string &message, void *param)
{
    sendInLoop(message);
}
