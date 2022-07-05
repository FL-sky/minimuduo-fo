//author voidccc

#include <errno.h>

#include "Epoll.h"
#include "Channel.h"
#include "Define.h"

#include <iostream>
using namespace std;

Epoll::Epoll()
{
    _epollfd = ::epoll_create(1);
    if (_epollfd <= 0)
        cout << "epoll_create error, errno:" << _epollfd << endl;
}

Epoll::~Epoll()
{}
//2 Epoll类的作用是包装epoll文件描述符，它最重要的成员变量是一个epoll文件描述符，最重要的两个方法是poll和update。
//poll方法包装了epoll_wait，在epoll描述符上等待事件的发生，当有事件发生后将新建的Channel填充到vector中，
//update方法包装了epoll_ctl，用来在epoll文件描述符上添加/修改/删除事件。
//update接收一个Channel作为参数，通过这个Channel可以获得要注册的事件(Channel::getEvents()方法)。
//以后所有涉及epoll描述符的操作都通过Epoll的这两个方法来完成。EventLoop本来是应该包括一个epoll描述符的，
//loop方法通过一个循环来调用epoll_wait，而现在epoll描述符在Epoll中，所以EventLoop只需要包含一个Epoll成员变量即可。
//EventLoop在循环中只需要调用Epoll::poll()方法就可以获得Channel列表，不需要直接调用epoll_wait了。


void Epoll::poll(vector<Channel*>* pChannels)
{
    int fds = ::epoll_wait(_epollfd, _events, MAX_EVENTS, -1);
    if(fds == -1)
    {
        cout << "epoll_wait error, errno:" << errno << endl;
        return;
    }
    for(int i = 0; i < fds; i++)
    {
        Channel* pChannel = static_cast<Channel*>(_events[i].data.ptr);
        pChannel->setRevents(_events[i].events);
        pChannels->push_back(pChannel);
    }
}

void Epoll::update(Channel* channel)
{
    struct epoll_event ev;
    ev.data.ptr = channel;
    ev.events = channel->getEvents();
    int fd = channel->getSockfd();
    ::epoll_ctl(_epollfd, EPOLL_CTL_ADD, fd, &ev);
}
