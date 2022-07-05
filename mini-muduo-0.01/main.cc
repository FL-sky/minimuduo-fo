#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h> //for bzero
#include <iostream>
//https://blog.csdn.net/voidccc/article/details/8721064
#include <unistd.h>
using namespace std;

#define MAX_LINE 100
#define MAX_EVENTS 500
#define MAX_LISTENFD 5

//mini-muduo v 0.01版本，这是mini-muduo的第一个版本，整个程序是一个100行的epoll示例

int createAndListen()
{
    int on = 1;
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    fcntl(listenfd, F_SETFL, O_NONBLOCK); //no-block io
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(11111);
    
    if(-1 == bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)))
    {
        cout << "bind error, errno:" << errno << endl; 
    }

    if(-1 == listen(listenfd, MAX_LISTENFD))
    {
        cout << "listen error, errno:" << errno << endl; 
    }
    return listenfd;
}

int main(int args, char** argv)
{
    struct epoll_event ev, events[MAX_EVENTS];
    int listenfd,connfd,sockfd;
    int readlength;
    char line[MAX_LINE];
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(struct sockaddr_in);

//    1 使用epoll的三个函数
//
//    int epoll_create(int size)
//    创建一个epoll文件描述符
//    int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
//            将socket描述符加入/移出epoll监听，修改注册事件
//    int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout)
//            在epoll描述符上等待事件的发生，并获得事件

    int epollfd = epoll_create(1);
    /*extern int epoll_create(int __size)
     * Creates an epoll instance.  Returns an fd for the new instance.
The "size" parameter is a hint specifying the number of file
descriptors to be associated with the new instance.  The fd
returned by epoll_create() should be closed with close().
     * */
    if (epollfd < 0)
        cout << "epoll_create error, error:" << epollfd << endl;
    listenfd = createAndListen();
    ev.data.fd = listenfd;
    ev.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);
/*extern int epoll_ctl(int __epfd, int __op, int __fd, epoll_event *__event)
 * Manipulate an epoll instance "epfd". Returns 0 in case of success,
-1 in case of error ( the "errno" variable will contain the
specific error code ) The "op" parameter is one of the EPOLL_CTL_*
constants defined above. The "fd" parameter is the target of the
operation. The "event" parameter describes which events the caller
is interested in and any associated user data.*/
    for(;;)
    {
        int fds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        /*extern int epoll_wait(int __epfd, epoll_event *__events, int __maxevents, int __timeout)
         * Wait for events on an epoll instance "epfd". Returns the number of
triggered events returned in "events" buffer. Or -1 in case of
error with the "errno" variable set to the specific error code. The
"events" parameter is a buffer that will contain triggered
events. The "maxevents" is the maximum number of events to be
returned ( usually size of "events" ). The "timeout" parameter
specifies the maximum wait time in milliseconds (-1 == infinite).

This function is a cancellation point and therefore not marked with
__THROW.*/
        if(fds == -1)
        {
            cout << "epoll_wait error, errno:" << errno << endl; 
            break;
        }
        for(int i = 0; i < fds; i++)
        {
            if(events[i].data.fd == listenfd)
            {
                connfd = accept(listenfd, (sockaddr*)&cliaddr, (socklen_t*)&clilen);
                if(connfd > 0)
                {
                    cout << "new connection from " 
                         << "[" << inet_ntoa(cliaddr.sin_addr) 
                         << ":" << ntohs(cliaddr.sin_port) << "]" 
                         << " accept socket fd:" << connfd 
                         << endl;
                }
                else
                {
                    cout << "accept error, connfd:" << connfd 
                         << " errno:" << errno << endl; 
                }
                fcntl(connfd, F_SETFL, O_NONBLOCK); //no-block io
                ev.data.fd = connfd;
                ev.events = EPOLLIN;
                if( -1 == epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev))
                    cout << "epoll_ctrl error, errno:" << errno << endl;
            }
            else if(events[i].events & EPOLLIN)
            {
                /*  https://blog.csdn.net/heluan123132/article/details/77891720
                 *  EPOLLIN       连接到达；有数据来临；
                    The associated file is available for read(2) operations.
                 * */
                if((sockfd = events[i].data.fd) < 0)
                {
                    cout << "EPOLLIN sockfd < 0 error " << endl;
                    continue;
                }
                bzero(line, MAX_LINE);
                if((readlength = read(sockfd, line, MAX_LINE)) < 0)
                {
                    if(errno == ECONNRESET)
                    {
                        cout << "ECONNREST closed socket fd:" << events[i].data.fd << endl;
                        close(sockfd);
                    } 
                }
                else if( readlength == 0)
                {
                    cout << "read 0 closed socket fd:" << events[i].data.fd << endl;
                    close(sockfd);
                }
                else
                {
                    if(write(sockfd, line, readlength) != readlength)
                        cout << "error: not finished one time" << endl;
                }
            }
        }
    }
    return 0;
}


