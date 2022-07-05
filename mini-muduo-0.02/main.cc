#include "TcpServer.h"
//mini-muduo v 0.02版本，这个版本添加的内容非常少，完整可运行的示例可从github下载，
//使用命令git checkout v0.02可切换到此版本，在线浏览到这里

//1 TcpServer作为第一个对象加入进来，以后main里面就只需要写几句简单的调用，
//主要逻辑从main函数移到了TcpServer里，一般来说全局只需要创建一个TcpServer的对象，
//然后通过调用TcpServer::start()启动整个服务器程序。


int main(int args, char** argv)
{
    TcpServer tcpserver;
    tcpserver.start();
    return 0;
}

//本版代码没太多好说的，正好借这个空记录下reactor模式，因为muduo就是使用的reactor模式，
//尽早了解这个模式可以帮助我们更好的理解代码。通过查看wikipedia，可以看到很多语言都有reactor模式的网络库，
//典型的比如Python写的Twisted。通过对比reactor和muduo的代码，发现muduo并没有完全按照文章介绍的类来组织程序，
//有一些小改动，后面每个版本增加新代码时会对照一下reactor的标准模板。
//
//我参考的reactor模式来源是Douglas C. Schmidt一篇paper，下面是pdf连接
//
//An Object Behavioral Pattern for Demultiplexing and Dispatching Handles for Synchronous Events
//http://www.dre.vanderbilt.edu/~schmidt/PDF/reactor-siemens.pdf
//
//文章对reactor模式进行了讲解，我把最主要的部分摘抄出来 note.txt

