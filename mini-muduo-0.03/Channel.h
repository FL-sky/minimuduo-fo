//author voidccc
#ifndef CHANNEL_H
#define CHANNEL_H

#include "Declear.h"
/*
 介绍一下Channel类，先看其声明，这里特别要注意_events和_revents，前者是要关注的事件，后者是发生的事件，不仔细看容易混淆。
 名字的来源是poll(2)的struct pollfd
 * */
class Channel
{
    public:
        Channel(int epollfd, int sockfd);
        ~Channel();
        void setCallBack(IChannelCallBack* callBack);
        void handleEvent();
        void setRevents(int revent);
        int getSockfd();
        void enableReading();
    private:
        void update();
        int _epollfd;
        int _sockfd;
        int _events;
        int _revents;
        IChannelCallBack* _callBack;
};

#endif
