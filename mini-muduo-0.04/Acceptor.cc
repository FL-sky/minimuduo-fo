//author voidccc

#include "Acceptor.h"
#include "Channel.h"
#include "IAcceptorCallBack.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include <iostream>
using namespace std;

Acceptor::Acceptor(int epollfd)
    :_epollfd(epollfd)
    ,_listenfd(-1)
    ,_pAcceptChannel(NULL)
    ,_pCallBack(NULL)
{}

Acceptor::~Acceptor()
{}

void Acceptor::start()
{
    _listenfd = createAndListen();
    _pAcceptChannel = new Channel(_epollfd, _listenfd); // Memory Leak !!!
    _pAcceptChannel->setCallBack(this);
    _pAcceptChannel->enableReading();
}

int Acceptor::createAndListen()
{
    int on = 1;
    _listenfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    fcntl(_listenfd, F_SETFL, O_NONBLOCK); //no-block io
    setsockopt(_listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(11111);

    if(-1 == bind(_listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)))
    {
        cout << "bind error, errno:" << errno << endl;
    }

    if(-1 == listen(_listenfd, MAX_LISTENFD))
    {
        cout << "listen error, errno:" << errno << endl;
    }
    return _listenfd;
}
//2 Acceptor类的作用就是处理新连接，Acceptor里的Channel关注着用于listen的socket，一旦有连接到达，
//就将连接accept下来（...这不废话嘛)。注意accept后还有个重要的步骤，就是回调一下TcpServer，
//使得后者有机会创建TcpConnection，这里为什么不直接在Acceptor里新建TcpConnection而是将其放到TcpServer里，
//我想在逻辑上新socket建立后就和之前用于listen的socket没有任何关系了，
//TcpConnection和Acceptor分别关心不同的socket上的不同事件，这两个类是平行的关系，
//如果前者创建后者，又牵扯生命周期的管理逻辑上不太合适，由TcpServer来掌握两者的生命周期更合适些。


void Acceptor::OnIn(int socket)
{
    int connfd;
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(struct sockaddr_in);
    connfd = accept(_listenfd, (sockaddr*)&cliaddr, (socklen_t*)&clilen);
    if(connfd > 0)
    {
        cout << "new connection from "
            << "[" << inet_ntoa(cliaddr.sin_addr) 
            << ":" << ntohs(cliaddr.sin_port) << "]"
            << " new socket fd:" << connfd
            << endl;
    }
    else
    {
        cout << "accept error, connfd:" << connfd
            << " errno:" << errno << endl;
    }
    fcntl(connfd, F_SETFL, O_NONBLOCK); //no-block io

    _pCallBack->newConnection(connfd);
}

void Acceptor::setCallBack(IAcceptorCallBack* pCallBack)
{
     _pCallBack = pCallBack;
}
