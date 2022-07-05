//author voidccc

#include <errno.h>

#include "TcpServer.h"
#include "Channel.h"
#include "Acceptor.h"
#include "TcpConnection.h"

#include <vector>
#include <iostream>

using namespace std;

TcpServer::TcpServer(EventLoop* loop)
    :_pAcceptor(NULL)
    ,_loop(loop)
{
}

TcpServer::~TcpServer()
{
}
//4 TcpServer也做了相应修改，在start方法里没有了epoll的创建过程，也没有了for循环。
//epoll的创建过程移动到Epoll的构造函数里。整个for循环都移动到EventLoop中，
//而真正启动循环的位置从TcpServer里移动到main函数里。TcpServer的另一处修改是将成员变量_epollfd换成EventLoop，
//因为EventLoop包含一个Epoll，而后者就是用来包装epoll文件描述符的，所以这个修改也很容易理解。

void TcpServer::start()
{
    _pAcceptor = new Acceptor(_loop); // Memory Leak !!!
    _pAcceptor->setCallBack(this);
    _pAcceptor->start();
}

void TcpServer::newConnection(int sockfd)
{
    TcpConnection* tcp = new TcpConnection(sockfd, _loop); // Memory Leak !!!
//    7 新增了一处内存泄漏new Epoll 后续处理。
    _connections[sockfd] = tcp;
}

//5 以前传递epollfd的地方，改为EventLoop* ，因为EventLoop和Epoll是一一对应的关系，
//而Epoll又是epollfd的包装，拿到EventLoop就等于拿到了epoll描述符。