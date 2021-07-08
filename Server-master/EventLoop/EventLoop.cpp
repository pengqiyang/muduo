
#include "EventLoop.h"
#include "../Log/Logging.h"
#include "../Encapsulate/Util.h"
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <iostream>
using namespace std;

__thread EventLoop* t_loopInThisThread = 0;

int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);//linux下一个eventfd用于读写
    if (evtfd < 0)
    {
        cout << "Failed in eventfd";
        abort();
    }
    cout<<"创建eventfd："<<evtfd<<endl;
    return evtfd;
}
//在线程函数中创建eventloop
EventLoop::EventLoop()
:   looping_(false),//是否在循环
    poller_(new Epoll()),//epoll轮询器
    wakeupFd_(createEventfd()),//事件描述符
    quit_(false),//是否关闭
    eventHandling_(false),//事件是否处理中
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),//当前线程ID
    WakeUpChannel(new Channel(this, wakeupFd_))
{
    cout<<"初始化eventloop ,并创建事件循环所需的epollfd和与主线程通信eventfd"<<endl;
    if (t_loopInThisThread)//one loop in one thread 避免重复创建
    {
    }
    else
    {
        t_loopInThisThread = this;
    }
    WakeUpChannel->setEvents(EPOLLIN | EPOLLET);
    WakeUpChannel->setReadHandler(bind(&EventLoop::handleRead, this));
    WakeUpChannel->setConnHandler(bind(&EventLoop::handleConn, this));
    poller_->epoll_add(WakeUpChannel, 0);
}

void EventLoop::handleConn()//处理连接//对于wakefdchannel来说，他的handleread和handlewrite没有什么作用
{
    updatePoller(WakeUpChannel, 0);
}


EventLoop::~EventLoop()
{
    close(wakeupFd_);
    t_loopInThisThread = NULL;
}

void EventLoop::wakeup()
{
    cout<<"调用wakeup()"<<endl;
    uint64_t one = 1;
    ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof one);
    if (n != sizeof one)
    {
        cout<< "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead()//对于wakefdchannel来说，他的handleread和handlewrite没有什么作用
{
    cout<<"调用read eventfd"<<endl;
    uint64_t one = 1;
    ssize_t n = readn(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        cout << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
    WakeUpChannel->setEvents(EPOLLIN | EPOLLET);
}
/*所有对IO和buffer的读写，都应该在IO线程中完成； runInLoop ()使得IO线程能够执行某个用户任务回调
//例如TCPConnection中检测send的时候，是否在当前IO线程，如果是的话，直接进行写相关操作sendInLoop。
//如果不在一个线程的话，需要将该任务抛给IO线程执行runInloop, 以保证write动作是在IO线程中执行的。我们后面会讲解runInloop的具体实现。
运行functor，将所有跨线程调用的函数转移到IO线程当中，实现线程安全，目的是为了唤醒阻塞在IO线程的空闲线程，让其执行其他非IO线程像执行的任务函数
*/
void EventLoop::runInLoop(Functor&& cb)
{
    cout<<"调用runInLoop"<<endl;
    if (isInLoopThread())
        cb();
    else
        queueInLoop(std::move(cb));
}

void EventLoop::queueInLoop(Functor&& cb)
{
    cout<<"调用queueInLoop"<<endl;
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.emplace_back(cb);//emplace_back比push_back效率高， 因为少了临时对象的内存空间申请以及拷贝构造函数
    }

    if (!isInLoopThread() || callingPendingFunctors_)
        wakeup();
}

void EventLoop::loop()//开始loop
{
    cout<<"开始loop"<<endl;
    assert(!looping_);
    assert(isInLoopThread());
    looping_ = true;
    quit_ = false;//开启状态
    std::vector<std::shared_ptr<Channel>> ret;
    while (!quit_)//循环
    {
        ret.clear();//清空
        ret = poller_->poll();//wait_epoll 获取就绪的事件
        eventHandling_ = true;//标志着事件处理中
        for (auto &it : ret) //C++ 11 for循环标准
            it->handleEvents();//依次处理事件
        eventHandling_ = false;//处理完毕
        doPendingFunctors();//TODO：？
        poller_->handleExpired();//处理超时问题
    }
    looping_ = false;
}

void EventLoop::doPendingFunctors()
{
    cout<<"调用doPendingFunctors"<<endl;
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        /*互斥锁的变种lock_guard,会自动构建互斥锁对象，当对象离开作用域之后就会调用析构函数，
        从而进行unlock,这种自动化的方式类似于之智能指针，避免忘记释放锁资源
        */
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);//尽快释放锁资源
    }

    for (size_t i = 0; i < functors.size(); ++i)
        functors[i]();
    callingPendingFunctors_ = false;
}

void EventLoop::quit()//关闭loop
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}
