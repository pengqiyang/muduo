//
#pragma once
#include "Thread.h"
#include "../Encapsulate/EpollPoller.h"
#include "../Log/Logging.h"
#include "Channel.h"
#include "../Encapsulate/Util.h"
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
using namespace std;

class EventLoop
{
public:
    typedef std::function<void()> Functor;
    EventLoop();
    ~EventLoop();
    void loop();
    void quit();
    void runInLoop(Functor&& cb);
    void queueInLoop(Functor&& cb);
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
    void assertInLoopThread()
    {
        assert(isInLoopThread());
    }
    void shutdown(shared_ptr<Channel> channel)
    {
        shutDownWR(channel->getFd());
    }
    void removeFromPoller(shared_ptr<Channel> channel)
    {

        poller_->epoll_del(channel);
    }
    void updatePoller(shared_ptr<Channel> channel, int timeout = 0)
    {
        poller_->epoll_mod(channel, timeout);
    }
    void addToPoller(shared_ptr<Channel> channel, int timeout = 0)
    {
        poller_->epoll_add(channel, timeout);
    }

private:
    // 声明顺序 wakeupFd_ > WakeUpChannel
    bool looping_;
    shared_ptr<Epoll> poller_;//main reactor（主ventloop）关注的套接字有wakefd和acceptfd,而sub reactor(即线程池)中关注的套接字都是connfd
    int wakeupFd_;
    bool quit_;
    bool eventHandling_;
    mutable MutexLock mutex_;
    std::vector<Functor> pendingFunctors_;
    bool callingPendingFunctors_;
    const pid_t threadId_;
    shared_ptr<Channel> WakeUpChannel;

    void wakeup();
    void handleRead();
    void doPendingFunctors();
    void handleConn();
};
