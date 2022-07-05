//author voidccc

#include "TcpServer.h"
#include "EventLoop.h"

//mini-muduo v0.05版本，完整可运行的示例可从github下载，使用命令git checkout v0.05可切换到此版本，
//在线浏览此版本到这里 本版将程序的主要类都加入进来了，这个版本可以作为一个里程碑版本，
//最重要的修改是加入了两个类EventLoop和Epoll。加入这两个类后，程序代码逻辑就相对清晰多了。
//为了有个更直观的了解，我们对照之前介绍的最简单epoll示例（从epoll构建muduo-2 最简单的epoll），
//看看原始示例里的关键代码现在都跑哪去了。


int main(int args, char** argv)
{
    EventLoop loop;
    TcpServer tcpserver(&loop);
    tcpserver.start();
    loop.loop();
    return 0;
}
