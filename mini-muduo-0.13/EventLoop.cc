// author voidccc

#include <sys/eventfd.h>

#include "EventLoop.h"
#include "Channel.h"
#include "Epoll.h"
#include "TimerQueue.h"
#include "Timestamp.h"
#include "Task.h"
#include "CurrentThread.h"

#include <iostream>
using namespace std;

EventLoop::EventLoop()
    : _quit(false), _callingPendingFunctors(false), _pPoller(new Epoll()) // Memory Leak !!!
      ,
      _threadId(CurrentThread::tid()), _pTimerQueue(new TimerQueue(this)) // Memory Leak!!!
{
    _eventfd = createEventfd();
    _pEventfdChannel = new Channel(this, _eventfd); // Memory Leak !!!
    _pEventfdChannel->setCallback(this);
    _pEventfdChannel->enableReading();
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

// 5 EventLoop

// 5.1 wakeup方法的实现在上一版本v0.12已经加入，但是调用被注释掉了。这次调用点位于EventLoop::queueInLoop。
// 这个方法是用唤醒IO线程的，确切的说是唤醒IO线程里的epoll_wait，
// 只有一点要注意，别忘记在EventLoop::handleRead里读出这个uint_64，否则会导致eventfd被持续激发使程序进入无限循环

void EventLoop::queueInLoop(Task &task)
{
    {
        MutexLockGuard guard(_mutex);
        _pendingFunctors.push_back(task);
    }

    if (!isInLoopThread() || _callingPendingFunctors)
    {
        wakeup();
    }
}

// 5.2 EventLoop::queueInLoop方法，这个方法在v0.12版本叫queueLoop，为了和原始muduo保持一致，本版改名了。
// 这个方法的作用是将一个异步回调加入到待执行队列_pendingFunctors中，
// 与v0.12版本相比第一个差异是本版本对_pendingFunctors加了锁，这点很好理解，
// 因为EventLoop::queueLoop经常被外部的其他非IO线程调用。
// 第二个修改是添加了一定条件下的wakeup()唤醒。为什么单线程版本没有这个唤醒逻辑？
// 因为单线程版本里所有的异步调用都是在Loop循环开始后，doPendingFunctors()之前触发的，只需要把回调插入_pendingFunctors这个数组即可。
// 但是在多线程版本queueInLoop的入口就很多了，比如下面这3种情况下，都可能调用EventLoop::queueInLoop

//         情况 1 IO线程，在IMuduoUser::onMessage回调里，比如EchoServer::onMessage里。

//         情况 2 IO线程，在doPendingFunctors()执行Task－>doTask的实现体里，比如EchoServer::onWriteComplate里。

//         情况 3 非IO线程，线程池的另一个线程中。

// 在单线程版本里，可以不考虑情况3，情况2虽然有可能发生，但是我们当时简单假设用户只会在onMessage添加Task，而不会在Task的回调里再添加Task。
// 所以上一个版本在此处进行了简单化处理，并不需要wakeup()操作。
// 本版本由于要考虑这几种情况，所以添加了一些条件判断和wakeup()调用。

// 特别要注意_callingPendingFunctors变量。
// 这个变量有点隐晦，我开始在写程序的时候忽略了它，后来发现它非常重要，在上面的情况2时，如果没有这个变量，会导致异步调用永远不会触发！
//

void EventLoop::runInLoop(Task &task)
{
    if (isInLoopThread())
    {
        task.doTask();
    }
    else
    {
        queueInLoop(task);
    }
}
//   5.3 EventLoop::runInLoop方法，本版本新添加的方法，
//   与queueInLoop方法非常相似，"runIn"和"queueIn"从名字的差异就可以理解，
//   当外部调用runInLoop的时候，会判断当前是否为IO线程，如果是在IO线程，则立刻执行Task里的回调。
//   否则通过调用queueInLoop将Task加入到异步队列，等待后续被调用。

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
    while (false)
        ;
}

//   5.4 EventLoop::doPendingFunctors，这个方法与queueInLoop方法一样，也是两处修改，
//   首先是由于多线程操作vector必须要加锁，
//   另外是添加了 _callingPendingFunctors 变量的控制，再次强调这个变量非常重要。

void EventLoop::doPendingFunctors()
{
    vector<Task> tempRuns;
    _callingPendingFunctors = true;
    {
        MutexLockGuard guard(_mutex);
        tempRuns.swap(_pendingFunctors);
    }
    vector<Task>::iterator it;
    for (it = tempRuns.begin(); it != tempRuns.end(); ++it)
    {
        it->doTask();
    }
    _callingPendingFunctors = false;
}
int EventLoop::runAt(Timestamp when, IRun0 *pRun)
{
    return _pTimerQueue->addTimer(pRun, when, 0.0);
}

int EventLoop::runAfter(double delay, IRun0 *pRun)
{
    return _pTimerQueue->addTimer(pRun, Timestamp::nowAfter(delay), 0.0);
}

int EventLoop::runEvery(double interval, IRun0 *pRun)
{
    return _pTimerQueue->addTimer(pRun, Timestamp::nowAfter(interval), interval);
}

void EventLoop::cancelTimer(int timerId)
{
    _pTimerQueue->cancelTimer(timerId);
}

bool EventLoop::isInLoopThread()
{
    return _threadId == CurrentThread::tid();
}
