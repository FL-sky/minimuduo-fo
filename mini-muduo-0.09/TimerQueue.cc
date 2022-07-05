// author voidccc

#include <sys/timerfd.h>
#include <inttypes.h>
#include <stdio.h>
#include <strings.h>

#include "TimerQueue.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Timestamp.h"

#include <iostream>
#include <unistd.h>
#define UINTPTR_MAX 0xffffffff

TimerQueue::TimerQueue(EventLoop *pLoop)
    : _timerfd(createTimerfd()), _pLoop(pLoop), _timerfdChannel(new Channel(_pLoop, _timerfd)) // Memory Leak !!!
      ,
      _addTimerWrapper(new AddTimerWrapper(this)) // Memory Leak !!!
      ,
      _cancelTimerWrapper(new CancelTimerWrapper(this)) // Memory Leak !!!
{
    _timerfdChannel->setCallback(this);
    _timerfdChannel->enableReading();
}

TimerQueue::~TimerQueue()
{
    ::close(_timerfd);
    /* int close(int __fd)
Close the file descriptor FD.

This function is a cancellation point and therefore not marked with
__THROW.
    */
}

void TimerQueue::doAddTimer(void *param)
{
    Timer *pTimer = static_cast<Timer *>(param);
    bool earliestChanged = insert(pTimer);
    if (earliestChanged)
    {
        resetTimerfd(_timerfd, pTimer->getStamp());
    }
}
// 这里调用了两个方法，一个是insert(...)，一个是resetTimerfd(...)，
// 在insert(...)里，程序将Timer插入到一个TimerList里，也就是一个std::set<std::pair<Timestamp, Timer*>>，看上去有点繁琐，
// 其实是一个 时间->Timer 键值对的set，这个set的目的是存放所有未到期的定时器，
// 每当timerfd到时，就从里面取出最近的一个定时器，然后修改timerfd把定时器修改成set里下一个最近的时间，
// 这样实现了只使用一个timerfd来管理多个定时器的功能。
// insert的返回值是个布尔型，意义是新加入的这个定时器是否否比整个set里所有定时器发生的还要早，
// 如果是的话，就必须立刻修改timerfd，将timerfd的定时时间改成这个最近的定时器。
// 如果新加入的定时器不是set里最先发生的定时器，则不用修改timerfd了。

void TimerQueue::doCancelTimer(void *param)
{
    Timer *pTimer = static_cast<Timer *>(param);
    Entry e(pTimer->getId(), pTimer);
    TimerList::iterator it;
    for (it = _timers.begin(); it != _timers.end(); ++it)
    {
        if (it->second == pTimer)
        {
            _timers.erase(it);
            break;
        }
    }
}

///////////////////////////////////////
/// Add a timer to the system
/// @param pRun: callback interface
/// @param when: time
/// @param interval:
///     0 = happen only once, no repeat
///     n = happen after the first time every n seconds
/// @return the process unique id of the timer
long TimerQueue::addTimer(IRun *pRun, Timestamp when, double interval)
{
    Timer *pTimer = new Timer(when, pRun, interval); // Memory Leak !!!
    _pLoop->queueLoop(_addTimerWrapper, pTimer);
    // return (int)pTimer;
    return (long)pTimer;
}
// 这里面新建了一个Timer对象，然后就调用了EventLoop::queueLoop(...)，
// 而queueLoop方法的作用就是异步执行(目前只有一个线程，所以只有异步执行的功能)。
// 异步执行了TimerQueue::doAddTimer(...)方法。再来看看 doAddTimer 方法

void TimerQueue::cancelTimer(long timerId) //关闭一个Timer
{
    _pLoop->queueLoop(_cancelTimerWrapper, (void *)timerId);
}

void TimerQueue::handleRead()
{
    Timestamp now(Timestamp::now());
    readTimerfd(_timerfd, now);

    vector<Entry> expired = getExpired(now);
    vector<Entry>::iterator it;
    for (it = expired.begin(); it != expired.end(); ++it)
    {
        it->second->run();
    }
    reset(expired, now);
}

void TimerQueue::handleWrite()
{
}

int TimerQueue::createTimerfd()
{
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                   TFD_NONBLOCK | TFD_CLOEXEC);
    // int timerfd_create(clockid_t __clock_id, int __flags)
    // Return file descriptor for new interval timer source.//创建一个定时器文件
    if (timerfd < 0)
    {
        cout << "failed in timerfd_create" << endl;
    }
    return timerfd;
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer *>(UINTPTR_MAX));
    TimerList::iterator end = _timers.lower_bound(sentry);
    copy(_timers.begin(), end, back_inserter(expired));
    _timers.erase(_timers.begin(), end);
    return expired;
}

void TimerQueue::readTimerfd(int timerfd, Timestamp now)
{
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));
    if (n != sizeof(howmany))
    {
        cout << "Timer::readTimerfd() error " << endl;
    }
}

void TimerQueue::reset(const vector<Entry> &expired, Timestamp now)
{
    vector<Entry>::const_iterator it;
    for (it = expired.begin(); it != expired.end(); ++it)
    {
        if (it->second->isRepeat())
        {
            it->second->moveToNext();
            insert(it->second);
        }
    }

    Timestamp nextExpire;
    if (!_timers.empty())
    {
        nextExpire = _timers.begin()->second->getStamp();
    }
    if (nextExpire.valid())
    {
        resetTimerfd(_timerfd, nextExpire);
    }
}

void TimerQueue::resetTimerfd(int timerfd, Timestamp stamp)
{
    struct itimerspec newValue;
    struct itimerspec oldValue;
    bzero(&newValue, sizeof(newValue));
    bzero(&oldValue, sizeof(oldValue));
    newValue.it_value = howMuchTimeFromNow(stamp);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    /* int timerfd_settime(int __ufd, int __flags, const itimerspec *__utmr, itimerspec *__otmr)
Set next expiration time of interval timer source UFD to UTMR. If
FLAGS has the TFD_TIMER_ABSTIME flag set the timeout value is
absolute. Optionally return the old expiration time in OTMR.
    */
    //设置新的超时时间，并开始计时
    if (ret)
    {
        cout << "timerfd_settime error" << endl;
    }
}

bool TimerQueue::insert(Timer *pTimer)
{
    bool earliestChanged = false;
    Timestamp when = pTimer->getStamp();
    TimerList::iterator it = _timers.begin();
    if (it == _timers.end() || when < it->first)
    {
        earliestChanged = true;
    }
    pair<TimerList::iterator, bool> result = _timers.insert(Entry(when, pTimer));
    if (!(result.second))
    {
        cout << "_timers.insert() error " << endl;
    }

    return earliestChanged;
}

struct timespec TimerQueue::howMuchTimeFromNow(Timestamp when)
{
    int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100)
    {
        microseconds = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(
        microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(
        (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}
