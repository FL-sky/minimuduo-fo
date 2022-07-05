// author voidccc

#include "TcpServer.h"
#include "EventLoop.h"
#include "EchoServer.h"
// mini-muduo v0.09版本，实现了Timer定时器。mini-muduo完整可运行的示例可从github下载，
// 使用命令git checkout v0.09可切换到此版本，在线浏览此版本到这里。
// 这个版本是一个里程碑版本，到这个版本为止，可以说一个单线程的Reactor模式网络库基本成型，
// 程序里的类各司其职，协同完成网络数据的首发。
// 后续版本只是在此基础上增加了多个工作线程(耗费CPU或需要等待磁盘读写的线程)和多个IO线程(IO multiplexing 的那个线程，
// 也就是跑select/poll/epoll的线程)。

int main(int args, char **argv)
{
    EventLoop loop;
    EchoServer echoserver(&loop);
    echoserver.start();
    loop.loop();
    return 0;
}
