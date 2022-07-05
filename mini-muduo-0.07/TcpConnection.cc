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
    : _sockfd(sockfd), _pLoop(pLoop), _pUser(NULL), _inBuf(new string()), _outBuf(new string())
{
    _pChannel = new Channel(_pLoop, _sockfd); // Memory Leak !!!
    _pChannel->setCallback(this);
    _pChannel->enableReading();
}

TcpConnection::~TcpConnection()
{
}
string outstr;
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
        _inBuf->append(line, readlength);
        // _pUser->onMessage(this, _inBuf);
        outstr = "0123456789";
        for (int i = 0; i < 22; i++)
            outstr += outstr;
        outstr += "\n";
        printf("outstr.size=%d\n", outstr.length());
        _pUser->onMessage(this, &outstr);
        puts("send finished!");
    }
}

void TcpConnection::handleWrite()
{
    int sockfd = _pChannel->getSockfd(); ///什么时候会触发这条语句
    if (_pChannel->isWriting())
    {
        int n = ::write(sockfd, _outBuf->c_str(), _outBuf->size());
        if (n > 0)
        {
            cout << "write " << n << " bytes data again" << endl;
            *_outBuf = _outBuf->substr(n, _outBuf->size());
            if (_outBuf->empty())
            {
                _pChannel->disableWriting();
                puts("send finished2!");
                ::write(sockfd, "FINISHED", sizeof("FINISHED"));
            }
        }
    }
}

int TcpConnection::send(const string &message)
{
    int n = 0;
    if (_outBuf->empty())
    {
        n = ::write(_sockfd, message.c_str(), message.size());
        if (n < 0)
            cout << "write error" << endl;
        else if (n == message.size())
            ::write(_sockfd, "FINISHED", sizeof("FINISHED"));
    }
    /// https://blog.csdn.net/hhhlizhao/article/details/71552588
    if (n < static_cast<int>(message.size())) ///什么时候会触发这条语句
    {                                         ///一次最大写入数据 小于 message.size
        *_outBuf += message.substr(n, message.size());
        // if (_pChannel->isWriting())
        // {
        //     //add EPOLLOUT
        //     _pChannel->enableWriting();
        // }
        _pChannel->enableWriting(); ///
    }
    return n;
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
