//author voidccc

#include "EchoServer.h"
#include "TcpConnection.h"

#include <iostream>

#define MESSAGE_LENGTH 20

EchoServer::EchoServer(EventLoop *pLoop)
    : _pLoop(pLoop), _pServer(pLoop)
{
    _pServer.setCallback(this);
}

EchoServer::~EchoServer()
{
}

void EchoServer::start()
{
    _pServer.start();
}

void EchoServer::onConnection(TcpConnection *pCon)
{
    cout << "onConnection" << endl;
}

// void EchoServer::onMessage(TcpConnection *pCon, string *data)
// {
//     while (data->size() > MESSAGE_LENGTH)
//     {
//         string message = data->substr(0, MESSAGE_LENGTH);
//         *data = data->substr(MESSAGE_LENGTH, data->size());
//         pCon->send(message + "\n");
//     }
// }
string message;
void EchoServer::onMessage(TcpConnection *pCon, string *data)
{
    while (data->size())
    {
        int rt = pCon->send(*data);
        printf("rt=%d sz-rt=%d\n", rt, data->size() - rt);
        if (data->size() != rt)
        {
            break;
        }
        *data = (data->substr(rt, data->size()));
    }
}