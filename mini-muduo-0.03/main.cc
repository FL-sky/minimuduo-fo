//author voidccc
#include "TcpServer.h"
//mini-muduo v 0.03版本，这是个版本最重要的修改是加入了一个名为Channel的类。
//完整可运行的示例可从github下载，使用命令git checkout v0.03可切换到此版本，在线浏览此版本到这里
int main(int args, char** argv)
{
    TcpServer tcpserver;
    tcpserver.start();
    return 0;
}
