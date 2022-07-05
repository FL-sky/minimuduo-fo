// author voidccc

#include <sys/eventfd.h>

#include "EventLoop.h"
#include "Channel.h"
#include "Epoll.h"
#include "TimerQueue.h"
#include "Timestamp.h"
#include <unistd.h>
#include <iostream>
using namespace std;

EventLoop::EventLoop()
    : _quit(false), _pPoller(new Epoll()) // Memory Leak !!!
      ,
      _pTimerQueue(new TimerQueue(this)) // Memory Leak!!!
{
    _eventfd = createEventfd();
    _wakeupChannel = new Channel(this, _eventfd); // Memory Leak !!!
    _wakeupChannel->setCallback(this);
    _wakeupChannel->enableReading();
}

EventLoop::~EventLoop()
{
}

void EventLoop::loop()
{
    while (!_quit)
    {
        vector<Channel *> channels;
        _pPoller->poll(&channels);

        vector<Channel *>::iterator it;
        for (it = channels.begin(); it != channels.end(); ++it)
        {
            (*it)->handleEvent();
        }

        doPendingFunctors();
    }
}

void EventLoop::update(Channel *pChannel)
{
    _pPoller->update(pChannel);
}

void EventLoop::queueLoop(IRun *pRun, void *param)
{
    Runner r(pRun, param);
    _pendingFunctors.push_back(r);
    wakeup();
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(_eventfd, &one, sizeof one);
    if (n != sizeof one)
    {
        cout << "EventLoop::wakeup() writes " << n << " bytes instead of 8" << endl;
    }
}

int EventLoop::createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        cout << "Failed in eventfd" << endl;
    }
    return evtfd;
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(_eventfd, &one, sizeof one);
    if (n != sizeof one)
    {
        cout << "EventEventLoop::handleRead() reads " << n << " bytes instead of 8" << endl;
    }
}

void EventLoop::handleWrite()
{
}

void EventLoop::doPendingFunctors()
{
    vector<Runner> tempRuns;
    tempRuns.swap(_pendingFunctors);
    vector<Runner>::iterator it;
    for (it = tempRuns.begin(); it != tempRuns.end(); ++it)
    {
        (*it).doRun();
    }
}

// 4 详细分析下Timer是怎么实现的，其实EventLoop里只有一个Timer文件描述符，
// 当用户通过上面的3个接口向EventLoop添加的所有定时器，实际都工作在同一个timerfd上，这个是怎么做到的呢？
// 我们来跟踪一下EventLoop::runAt()的实现
long EventLoop::runAt(Timestamp when, IRun *pRun) //在指定的某个时刻调用函数
{
    // EventLoop::runAt(...)直接调用了TimerQueue::addTimer()
    return _pTimerQueue->addTimer(pRun, when, 0.0);
}

long EventLoop::runAfter(double delay, IRun *pRun) //等待一段时间后，调用函数
{
    return _pTimerQueue->addTimer(pRun, Timestamp::nowAfter(delay), 0.0);
}

long EventLoop::runEvery(double interval, IRun *pRun) //以固定的时间间隔反复调用函数
{
    return _pTimerQueue->addTimer(pRun, Timestamp::nowAfter(interval), interval);
}

void EventLoop::cancelTimer(long timerId)
{
    _pTimerQueue->cancelTimer(timerId);
}
