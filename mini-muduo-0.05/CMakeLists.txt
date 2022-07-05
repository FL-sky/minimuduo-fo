cmake_minimum_required(VERSION 3.16)
project(v0_0)

set(CMAKE_CXX_STANDARD 14)

add_executable(v0_0
        main.cc Acceptor.cc Channel.cc Epoll.cc EventLoop.cc TcpConnection.cc TcpServer.cc)
