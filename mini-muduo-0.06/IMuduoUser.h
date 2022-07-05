//author voidccc
#ifndef IMUDUOUSER_H
#define IMUDUOUSER_H

#include "Declear.h"
#include <string>
using namespace std;

class IMuduoUser
{
    public:
        void virtual onConnection(TcpConnection* pCon) = 0;
        void virtual onMessage(TcpConnection* pCon, const string& data) = 0;
};

#endif

//2 IMuduoUser里目前有两个纯虚函数，onConnection和onMessage，分别在连接建立和收到数据的时候被调用，
//目前IMuduoUser接口还有两个地方待完善，第一，还有一个重要的接口没有加进来，那就是onWriteComplate，
//因为这个回调的实现稍微有点复杂，这个回调会在v0.08版本加入进来，到时再详细介绍。第二，onMessage回调的参数还很原始，
//onMessage担负着输送数据给用户的重任，当前版本只用了一个const string&来表示数据而不像muduo一样有专门的Buffer类，
//这个缺陷也会在v0.08版本得到完善。观察IMuduoUser的两个方法，发现都将TcpConnection传递给了用户，是的，
//这个类以后就是用户的指挥棒了，用户想发送数据或者做点别的操作，都是通过TcpConnection来完成的。目前最重要的就是send方法。
