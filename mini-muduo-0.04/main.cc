//author voidccc
#include "TcpServer.h"
//mini-muduo v0.04版本，这是个版本最重要的修改是加入了两个类Acceptor和TcpConnection。
//完整可运行的示例可从github下载，使用命令git checkout v0.04可切换到此版本，在线浏览此版本到这里
int main(int args, char** argv)
{
    TcpServer tcpserver;
    tcpserver.start();
    return 0;
}
//1 回顾下v0.03版本，所有的socket事件都是在TcpServer::OnIn(命名不规范后续会改动)里处理的，
//包括接受新连接和读写数据，那实在是没有办法的办法，现在加入了Acceptor和TcpConnection，
//终于可以将事件分配到独立的类处理了，这也是应该的，因为接受连接和读写数据根本就是两种性质的事件，
//处理方式也完全不同，理应放到两个不同的类中。

//4 回顾一下之前介绍过的reactor模型，对照我们新添加的两个类，
//其实Acceptor和TcpConnection就是两个Concrete Event Handler，在的Douglas C. Schmidt文档里，
//这两个Handler一个叫做Logging Acceptor，另一个叫做Logging Handler，
//名字和我们的有些不同，个人觉得muduo里的名字更好理解。
//
//5 只顾低头拉车不顾抬头看路，这个版本引入了4个Memory leak(代码中都有明确注释)，
//不过我想C++程序员是最有办法管理对象生命周期的，我们先忽略它。
//
//6 这个版本还是问题多多，后续不断改进。