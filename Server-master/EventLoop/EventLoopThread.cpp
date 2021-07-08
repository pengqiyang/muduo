
#include "EventLoopThread.h"
#include <functional>


EventLoopThread::EventLoopThread()
:   loop_(NULL),//空循环事件
    exiting_(false),//开始
    thread_(bind(&EventLoopThread::threadFunc, this), "EventLoopThread"),//指明事件循环线程的具体功能
    mutex_(),
    cond_(mutex_)
{ }

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != NULL)
    {
        loop_->quit();//结束循环
        thread_.join();//处理结束thread，回收资源
    }
}

/*

该函数的作用是启动EventLoopThread中的线程去启动EventLoop，然后把EventLoop对象的指针返给调用者，
由于启动线程，它和thread_的worker线程threadFunc是并发执行的，所以它需要等待loop产生成功才能返回loop的指针。
loop是I/O线程最重要
*/


EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.started());//保证不会多次开启
    thread_.start();//启动线程
    {
        MutexLockGuard lock(mutex_);//你要是不加锁，可能返回的loop_是个空，这里的loop_是共享资源
        // 一直等到threadFun在Thread里真正跑起来
        while (loop_ == NULL)
            cond_.Wait();
    }
    return loop_;
}

void EventLoopThread::threadFunc()//线程的功能函数
{
    EventLoop loop;

    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.Signal();
    }

    loop.loop();//开始事件循环
    loop_ = NULL;
}
