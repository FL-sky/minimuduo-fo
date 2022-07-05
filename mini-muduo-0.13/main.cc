// author voidccc

#include "TcpServer.h"
#include "EventLoop.h"
#include "EchoServer.h"

// mini-muduo v0.13版本，mini-muduo完整可运行示例可从github下载，使用命令git checkout v0.13可切换到此版本，在线浏览此版本到这里
// 本版是个里程碑版本，可以通过本版了解多线程是如何通过IO线程读/写网络数据的，在前一个版本v0.12重点介绍了基础知识的前提下，
// 本篇着重分析多线程逻辑里最重要的三个方法EventLoop::runInLoop／EventLoop::queueInLoop／EventLoop::doPendingFunctors。
// 下面逐步介绍本版本修改的细节，三个方法放在最后的EventLoop节。

int main(int args, char **argv)
{
    EventLoop loop;
    EchoServer echoserver(&loop);
    echoserver.start();
    loop.loop();
    return 0;
}
