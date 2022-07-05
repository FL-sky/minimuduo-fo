//author voidccc

#include "TcpServer.h"
#include "EventLoop.h"
#include "EchoServer.h"

//mini-muduo v0.07版本，这个版本是加入了发送缓冲区和接收缓冲区的初始版本，后续v0.08完善了缓冲区的实现。
//mini-muduo完整可运行的示例可从github下载，使用命令git checkout v0.07可切换到此版本，在线浏览此版本到这里

int main(int args, char** argv)
{
    EventLoop loop;
    EchoServer echoserver(&loop);
    echoserver.start();
    loop.loop();
    return 0;
}
