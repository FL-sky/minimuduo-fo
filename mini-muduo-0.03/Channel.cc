// author voidccc
#include "Channel.h"
#include "IChannelCallBack.h"
#include <sys/epoll.h>
#include <iostream>

Channel::Channel(int epollfd, int sockfd)
    : _epollfd(epollfd), _sockfd(sockfd), _events(0), _revents(0), _callBack(NULL)
{
}

void Channel::setCallBack(IChannelCallBack *callBack)
{
    _callBack = callBack;
}

void Channel::setRevents(int revents)
{
    _revents = revents;
}

void Channel::handleEvent()
{
    if (_revents & EPOLLIN)
        _callBack->OnIn(_sockfd);
}

int Channel::getSockfd()
{
    return _sockfd;
}

void Channel::enableReading()
{
    _events |= EPOLLIN;
    update();
}

// v0.01版本是这样的

//  50     ev.data.fd = listenfd;
//  51     ev.events = EPOLLIN;
//  52     epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);

// 当前版本
void Channel::update()
{
    struct epoll_event ev;
    ev.data.ptr = this;
    ev.events = _events;
    epoll_ctl(_epollfd, EPOLL_CTL_ADD, _sockfd, &ev);
}

// 差别在于ev.data.fd = listenfd 和 ev.data.ptr = this
// 使用man epoll_ctl来查看一下epoll_event的定义，data字段是一个union，所以可以存放任何64位长度的内容。

// data字段的作用是当事件发生后，可以让epoll的使用者获得这个事件的信息，v0.01版本只保存了一个sockef描述符在里面，
// 这样我们必须通过再额外建立一个"描述符->回调"映射才能找到注册的函数，然后去调用它。v0.03直接将Channel的指针保存到data字段中，
// 直接解决了这个问题，因为一个Channel对象可以保存任意的信息，回调放到其成员变量中即可，当然了Channel中也保存着socket描述符。