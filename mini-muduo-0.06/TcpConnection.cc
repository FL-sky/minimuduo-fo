//author voidccc

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

TcpConnection::TcpConnection(int sockfd, EventLoop* pLoop)
    :_sockfd(sockfd)
    ,_pLoop(pLoop)
    ,_pUser(NULL)
{
    _pChannel = new Channel(_pLoop, _sockfd); // Memory Leak !!!
    _pChannel->setCallback(this);
    _pChannel->enableReading();
}

TcpConnection::~TcpConnection()
{}
//3 因为响应请求的过程从TcpConnection移动到了EchoServer中，所以TcpConnection也做了相应更改，
//在原来的onIn方法里不再直接调用write将数据传回，
//而是在其中调用了其保存的IMuduoUser::onMessage方法将数据传递给了Muduo网络库的真正用户。
//同时TcpConnection增加了一个send方法，这个方法包装了write操作，提供给网络库的用户来使用。
//onMessage()方法的参数里会传递TcpConnection给用户，用户可以自行决定是否发送数据。

void TcpConnection::onIn(int sockfd)
{
    int readlength;
    char line[MAX_LINE];
    if(sockfd < 0)
    {
        cout << "EPOLLIN sockfd < 0 error " << endl;
        return;
    }
    bzero(line, MAX_LINE);
    if((readlength = read(sockfd, line, MAX_LINE)) < 0)
    {
        if(errno == ECONNRESET)
        {
            cout << "ECONNREST closed socket fd:" << sockfd << endl;
            close(sockfd);
        }
    }
    else if(readlength == 0)
    {
        cout << "read 0 closed socket fd:" << sockfd << endl;
        close(sockfd);
    }
    else
    {
        string buf(line, MAX_LINE);
        _pUser->onMessage(this, buf);
    }
}

void TcpConnection::send(const string& message)
{
    int n = ::write(_sockfd, message.c_str(), message.size());
    if( n != static_cast<int>(message.size()))
        cout << "write error ! " << message.size() - n << "bytes left" << endl;
}

void TcpConnection::connectEstablished()
{
    if(_pUser)
        _pUser->onConnection(this);
}

void TcpConnection::setUser(IMuduoUser* user)
{
    _pUser = user;
}
