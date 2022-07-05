#include "Task.h"

#include "IRun.h"

// 1 Task类

//     这个类是v0.13版本新加入的，它就是一个携带参数的回调。它的作用就是闭包(closure)，
//     它是我们用来代替muduo里boost::function和boost:bind的。
//     为什么不使用boost::function和boost:bind？之前解释过了，为了不引入新概念，降低mini-muduo的学习成本。
//     这个Task类不太具有通用性（不像BlockingQueue，范型实现），
//     只是为了在本项目里使用的，Task只支持两种类型的回调，第一种是无参数的回调，被调用者只需要实现一个”void run0()“，
//     第二种是有两个参数的回调，被调用者实现"void run2(const string&, void*)"。
//     因为有了Task类，所有需要异步回调的地方都用它来实现了。

Task::Task(IRun0 *func)
    : _func0(func), _func2(NULL), _param(NULL)
{
}

Task::Task(IRun2 *func, const string &str, void *param)
    : _func0(NULL), _func2(func), _str(str), _param(param)
{
}

void Task::doTask()
{
    if (_func0)
    {
        _func0->run0();
    }
    else
    {
        _func2->run2(_str, _param);
    }
}
