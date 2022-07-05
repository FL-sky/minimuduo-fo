//author voidccc

#include "EventLoop.h"
#include "Channel.h"
#include "Epoll.h"

EventLoop::EventLoop()
    :_quit(false)
    ,_poller(new Epoll()) // Memory Leak !!!
{
}

EventLoop::~EventLoop()
{}
//1 先来看EventLoop，根据名字可以猜测这个类的作用是事件循环，其实这个类就是用来包装for循环的，
//也就是那个套在epoll_wait外面的for循环，这个for循环可以说是整个程序最核心的部分，for循环等待在epoll_wait上，
//然后遍历返回的每个事件，先通知到Channel，然后由Channel通知到最终的事件处理程序（位于Acceptor和TcpConnection中）。
//在上一个版本中。for循环位于TcpServer里，现在我们把它移动到EventLoop的loop方法里，用while代替for，
//作用跟之前的for循环一样：等待在epoll_wait上，当有事件发生时，回调Channel。
//当然，EventLoop::loop()不是直接调用epoll_wait，而是使用了其包装类Epoll。


void EventLoop::loop()
{
    while(!_quit)
    {
        vector<Channel*> channels;
        _poller->poll(&channels);

        vector<Channel*>::iterator it;
        for(it = channels.begin(); it != channels.end(); ++it)
        {
            (*it)->handleEvent(); 
        }
    }
}

//3 Epoll和EventLoop应该是一一对应的关系，每个EventLoop有且只有一个Epoll。
//在原始的muduo中，为了兼顾epoll/poll编程，作者为IO复用写了公共父类Poller，这里我进行了简化，
//直接实现了一个epoll的包装类Epoll而忽率了poll。

void EventLoop::update(Channel* channel)
{
    _poller->update(channel);
}
