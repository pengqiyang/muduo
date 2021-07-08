#pragma once
#include "../EventLoop/EventLoop.h"
#include "../EventLoop/Channel.h"
#include "../EventLoop/EventLoopThreadPool.h"
#include <memory>
class Connection
{
public:

    Connection(EventLoop *loop,  string addr, int port, int connfd);
    void messagecallback();


private:
    EventLoop *loop_;
    int connFd_;
    std::shared_ptr<Channel> connChannel_;
    string addr_;
    int port_;
};
