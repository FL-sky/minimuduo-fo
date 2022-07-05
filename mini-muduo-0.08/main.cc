// author voidccc

#include "TcpServer.h"
#include "EventLoop.h"
#include "EchoServer.h"

// mini-muduo v0.08版本，这个版本完善了缓冲区的实现。
// mini-muduo完整可运行的示例可从github下载，使用命令git checkout v0.08可切换到此版本，在线浏览此版本到这里

//  本版本有两处重要修改，首先实现了IMuduoUser的onWriteComplate回调，这样当用户一次传送大量数据到网络库时，用户会在数据发送完成后收到通知。
//  当然了发送小量数据完成时也会收到通知。
//  其次本版本实现了专门用于表示缓冲区的Buffer类，只不过这个Buffer类的细节实现非常简单。

int main(int args, char **argv)
{
    EventLoop loop;
    EchoServer echoserver(&loop);
    echoserver.start();
    loop.loop();
    return 0;
}
