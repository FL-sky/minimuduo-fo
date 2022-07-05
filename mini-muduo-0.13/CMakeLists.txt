cmake_minimum_required(VERSION 3.16)
project(v0_0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread" )
add_executable(v0_0
        main.cc Acceptor.cc Buffer.cc Channel.cc EchoServer.cc Epoll.cc EventLoop.cc Task.cc TcpConnection.cc TcpServer.cc Thread.cc ThreadPool.cc TimerQueue.cc Timestamp.cc)
