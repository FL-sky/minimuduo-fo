//author voidccc

#include "TcpServer.h"
#include "EventLoop.h"
#include "EchoServer.h"
//mini-muduo v0.06版本，这个版本应该算是一个里程碑版本，最重要的修改是将库和库的用户分离。完整可运行的示例可从github下载，
//使用命令git checkout v0.06可切换到此版本，在线浏览此版本到这里

int main(int args, char** argv)
{
    EventLoop loop;
    EchoServer echoserver(&loop);
    echoserver.start();
    loop.loop();
    return 0;
}

//1 在这个版本中，我们的网络库才真正的能被称为“库”。整个mini-muduo程序现在被分成了两部分，一边是库，另一边是库的用户。
//EchoServer这个类就是用户了，用户关心的是接到客户端的数据后如何处理，然后返回什么内容给客户端。而库要做的是顺利接收客户端数据，
//并将收到的数据送给用户处理，同时在用户发出发送指令后能将数据完整的发送给客户端。作为mini-muduo的用户，必须实现IMuduoUser接口，
//这个接口是网络库和用户之间的桥梁。这个接口和原始muduo几乎是一样的，唯一的一点差别是实现方式，
//在原始的muduo中，作者用boost::function和boost:bind取代虚函数，
//感兴趣的读者可以参考这篇文章(以boost::function和boost:bind取代虚函数)。

//4 我找了muduo原始代码里的一个例子，这是一个echo服务器位于muduo/example/sample/echo 这是很简单的例子了，
//可以看到muduo的用户是这样使用muduo库的

//int main1()
//{
//    LOG_INFO << "pid = " << getpid();
//    muduo::net::EventLoop loop;
//    muduo::net::InetAddress listenAddr(2007);
//    EchoServer server(&loop, listenAddr);
//    server.start();
//    loop.loop();
//}
//
//再看看我们的main.cc，已经与原始muduo非常接近了。

//EchoServer类我就不粘贴对比代码了，也与muduo非常相似。
//5 本版本还修正了一些命名规范问题，把接口头文件里的虚函数换成纯虚纯函数，这都是之前代码的一些疏忽。
//
//6 本版本依然是问题多多，比如读取和发送操作实现的都非常简单，几乎还处于不可用状态。
//内存泄漏一大堆，单线程有可能阻塞等等问题，后续版本逐步解决这些问题
