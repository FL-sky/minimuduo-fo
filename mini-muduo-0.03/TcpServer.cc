// author voidccc
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include "TcpServer.h"
#include "Channel.h"

#include <string.h> //for bzero
#include <iostream>
#include <vector>
#include <unistd.h>
using namespace std;

TcpServer::TcpServer()
    : _epollfd(-1), _listenfd(-1)
{
}

TcpServer::~TcpServer()
{
}

int TcpServer::createAndListen()
{
    int on = 1;
    _listenfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    fcntl(_listenfd, F_SETFL, O_NONBLOCK); // no-block io
    setsockopt(_listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(11111);

    if (-1 == bind(_listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)))
    {
        cout << "bind error, errno:" << errno << endl;
    }

    if (-1 == listen(_listenfd, MAX_LISTENFD))
    {
        cout << "listen error, errno:" << errno << endl;
    }
    return _listenfd;
}

void TcpServer::OnIn(int sockfd)
{
    cout << "OnIn:" << sockfd << endl;
    if (sockfd == _listenfd)
    {
        int connfd;
        struct sockaddr_in cliaddr;
        socklen_t clilen = sizeof(struct sockaddr_in);
        connfd = accept(_listenfd, (sockaddr *)&cliaddr, (socklen_t *)&clilen);
        if (connfd > 0)
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
        fcntl(connfd, F_SETFL, O_NONBLOCK); // no-block io

        // Memory Leak !!!
        Channel *pChannel = new Channel(_epollfd, connfd);
        //        按照作者描述"每个Channel对象自始至终只负责一个文件描述符的IO事件分发"。
        //        我是这么理解的，Channel把socket文件描述符和关心这个描述符的回调捆绑在了一起，
        //        之前的v0.01版本，程序在调用epoll_wait获得事件后，直接就进行了事件处理，
        //        现在通过添加Channel，程序终于可以将事件处理程序写在一个单独的函数中，然后将这个函数注册到Channel上。
        //        这个注册的过程比较关键，在TcpServer.cc的117和118行，下面这两句调用
        // 117
        pChannel->setCallBack(this);
        // 118
        pChannel->enableReading();
        //        117行负责把回调指针传递给Channel(muduo使用的是boost::function)，
        //        这个指针指向实现IChannelCallback接口的一个对象，也就是TcpServer本身了。
        //        setCallBack只是将这个指针保存在成员变量_callBack中，等待后续调用。
        //        118行enableReading()的实现有两步，首先将_events(注意和_revents区别)里加入EPOLLIN标记，
        //        然后通过update()将事件真正注册到epollfd上，这是最关键的步骤。调用epoll_ctl注册的过程和v0.01版本有微小的差别，
        //        这个差别也很关键。
    }
    else
    {
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
            if (write(sockfd, line, readlength) != readlength)
                cout << "error: not finished one time" << endl;
        }
    }
}

void TcpServer::start()
{
    _epollfd = epoll_create(1);
    if (_epollfd <= 0)
        cout << "epoll_create error, errno:" << _epollfd << endl;
    _listenfd = createAndListen();

    Channel *pChannel = new Channel(_epollfd, _listenfd);
    pChannel->setCallBack(this);
    pChannel->enableReading();

    for (;;)
    {
        vector<Channel *> channels;
        int fds = epoll_wait(_epollfd, _events, MAX_EVENTS, -1);
        if (fds == -1)
        {
            cout << "epoll_wait error, errno:" << errno << endl;
            break;
        }
        // 在epoll_wait()返回事件后，有两块逻辑要执行
        //
        // 129
        for (int i = 0; i < fds; i++)
        {
            Channel *pChannel = static_cast<Channel *>(_events[i].data.ptr);
            pChannel->setRevents(_events[i].events);
            /* https://blog.csdn.net/sjin_1314/article/details/8240511
             * struct epoll_event {
         __uint32_t events;      //* epoll event
            epoll_data_t data;      //* User data
        };
             *
             * 其中events表示感兴趣的事件和被触发的事件，可能的取值为：
   EPOLLIN：表示对应的文件描述符可以读；
   EPOLLOUT：表示对应的文件描述符可以写；
   EPOLLPRI：表示对应的文件描述符有紧急的数可读；

   EPOLLERR：表示对应的文件描述符发生错误；
    EPOLLHUP：表示对应的文件描述符被挂断；
   EPOLLET：    ET的epoll工作模式；
             * */
            channels.push_back(pChannel);
        } // 134
        //第一步129行到134行 ，遍历所有的事件，从其data字段中拿出和这个socket相关的Channel指针，
        // 并且将其_revents(注意和_events区别)字段填充好，最后将Channel插入到vector中
        //
        // 136
        vector<Channel *>::iterator it;
        for (it = channels.begin(); it != channels.end(); ++it)
        {
            (*it)->handleEvent();
        } // 140
        //第二步136行到140行，遍历vector，逐一调用其中的handleEvent方法。
        // handleEvent方法里会直接调用_callBack的OnIn方法，把事件送给注册好的回调进行处理。
        //
    }
}

// 这里之所以分成两个步骤而不是一边遍历fds一边调用handleEvent()，是由于后者会添加或删除Channel，
// 从而造成fds在遍历期间改变大小，这是非常危险的。作者在书中有提及(P285)。

// v0.03版本重要修改介绍完了，下面是一些小改动和注意事项

// 1加入了防止头文件重复引用的#ifndef系列宏

// 2加入前置声明 和专门用于前置声明的Declear.h和宏定义Define.h

// 3 引入了内存泄漏，代码中有注释。
// 经过这个版本，代码的问题还很多，后续改进。
