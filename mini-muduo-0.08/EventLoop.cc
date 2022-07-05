// author voidccc

#include <sys/eventfd.h>

#include "EventLoop.h"
#include "Channel.h"
#include "Epoll.h"

#include <iostream>
#include <unistd.h>
using namespace std;

EventLoop::EventLoop()
    : _quit(false), _pPoller(new Epoll()) // Memory Leak !!!
{
    _eventfd = createEventfd();
    _wakeupChannel = new Channel(this, _eventfd); // Memory Leak !!!
    _wakeupChannel->setCallback(this);
    _wakeupChannel->enableReading();
}

EventLoop::~EventLoop()
{
}

// 2.3 真正的事件处理过程位于EventLoop::loop()方法中，新添加的EventLoop::doPendingFunctors()方法正是用来触发异步调用的。
// 现在重新理顺一下EventLoop::queueLoop()方法的实现，这个方法其实就是先将一个代表回调的IRun*放入到EventLoop的vector中保存，
// 然后就触发eventfd的事件，本次循环完毕，当下次EventLoop::loop循环到epoll_wait时，会因为eventfd的触发而返回，
// 这时eventfd对应的Channel会被通知，进而通知到EventLoop::handleRead方法，我们在里面把事件读出来，这样确保事件只触发一次。
// EventLoop::loop循环会继续调用到doPendingFunctors()方法，这里面遍历保存IRun*的vector，于是所有异步事件就开始处理了。

// EventLoop::loop()方法
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
        // 3 注意doPendingFunctors方法的实现，这里不是通过简单的遍历vector来调用回调，
        // 而是新建了一个vector，然后调用vector::swap方法将数组交换出来后再调用，这么做的目的是“减小临界区的长度和避免死锁”，
        // 在<<Linux多线程服务器端编程>>P295页有详细介绍。当然我们的mini-muduo目前还是单线程，影响不大。
    }
}

void EventLoop::update(Channel *pChannel)
{
    _pPoller->update(pChannel);
}

// 2 EventLoop::queueLoop()方法，本版本新添加的方法，这是一个非常非常重要的方法。
// 在没有这个方法之前，我们使用epoll_wait()接收的所有事件，都是来自操作系统的，
// 比如EPOLLIN/EPOLLOUT，我们使用epollfd只是用来接收操作系统的网络通知。
// 现在我们需要让epollfd多做点事情，让他能接收网络库自己发送的通知。这种通知有两个重要的价值

//     价值1： 使得网络库内部在单线程的情况下具备异步处理事件的能力。

//     价值2： 使得网络库的IO线程(跑epoll_wait的那个线程)，可以接收来自非本线程的发送请求。

// 这种通知正是通过eventfd机制实现的，eventfd由Linux 2.6.22新引入，可以像socket一样被epollfd所监听，
// 如果向eventfd写点东西，epoll就会获得这个通知并返回。EventLoop正是通过封装eventfd才获得了异步处理能力。

// EventLoop::queueLoop()方法
void EventLoop::queueLoop(IRun *pRun)
{
    _pendingFunctors.push_back(pRun);
    wakeup();
}
//--
// 4 之所以通过EventLoop::queueLoop()来异步触发onWriteComplate而不是直接在TcpConnection里触发onWriteComplate，
// 我想是为了防止回调嵌套，因为我们在onMessage里调用了TcpConnection::send()方法，
// 如果onWriteComplate又是直接在send里被调用的话，就会导致onMessage嵌套调用了onWriteComplate，
// 这样事件的序列性被打破，会引入一堆问题。

//--

// EventLoop::wakeup()里调用了::wirte()
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(_eventfd, &one, sizeof one);
    puts("write(_eventfd");
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
    puts("createEventfd");
    return evtfd;
}
// EventLoop::handleRead()里调用了::read()
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(_eventfd, &one, sizeof one);
    puts("read(_eventfd");
    if (n != sizeof one)
    {
        cout << "EventEventLoop::handleRead() reads " << n << " bytes instead of 8" << endl;
    }
}

void EventLoop::handleWrite()
{
    puts("EventLoop::handleWrite");
}

void EventLoop::doPendingFunctors()
{
    vector<IRun *> tempRuns;
    tempRuns.swap(_pendingFunctors);
    vector<IRun *>::iterator it;
    for (it = tempRuns.begin(); it != tempRuns.end(); ++it)
    {
        (*it)->run();
    }
}

//===========
// 5 因为目前程序都跑在一个进程的唯一线程中，muduo中的所有线程相关代码还未加入，
// 当后面版本多线程被加入进来后，一些关键数据要被mutex保护起来。

// 6 将const string& 和 string* 都换成Buffer，保持和muduo一致，Buffer里只实现了几个基本方法，
// 比如append()只实现了const string&版本而没有(const char* data, int len)版本，
// Buffer使用了std::string作为自己的存储介质，方法实现也比较粗糙，效率比较差，好处是简单易懂而且和原始muduo有相同的接口。
// muduo里的Buffer设计作者花费了一些心思，使用了栈和堆结合的方法，在书中7.4节已经进行了详细的介绍。
// Buffer::retrieve方法的作用是丢弃掉缓冲区里前n个字节。mini-muduo没有使用定制的string类而是直接使用了std::string。

// 7 muduo里另外的关于缓冲区的回调我在mini-muduo里没有实现，个人认为这不会影响到对基础框架的理解。
// 比如“高水位回调”HighWaterMarkCallback，如果输出缓冲的长度超过用户制定的大小会触发。对这个回调的实现有兴趣的同学可参看muduo代码。
