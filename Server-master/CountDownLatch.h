
#pragma once
#include "Encapsulate/Condition.h"
#include "Encapsulate/MutexLock.h"
#include "Encapsulate/NoCopy.h"

/*CountDownLatch的主要作用是确保Thread中传进去的func真的启动了以后外层的start才返回
概念定义：条件变量是对互斥锁的改进，防止线程陷入busy_wait,浪费CPU 资源，所以引入条件变量，当没有达到某个线程的运行条件时，就sleep,防止其无效竞争锁，影响其他线程的执行
使用场景1：主线程发起多个子线程，等这些子线程各自都完成一定的任务之后，主线程才继续执行。通常用于主线程等待多个子线程完成初始化。
使用场景2：主线程发起多个子线程，子线程都等待主线程，主线程完成其他一些任务之后通知所有子线程开始执行。通常用于多个子线程等待主线程发出“起跑”命令。

主线程发起多个子线程，子线程都等待主线程，主线程完成其他一些任务之后通知所有子线程开始执行。通常用于多个子线程等待主线程发出“起跑”命令。
*/
class CountDownLatch : NoCopy
{
public:
    explicit CountDownLatch(int count);
    void wait();
    void countDown();

private:
    mutable MutexLock mutex_;
    Condition condition_;
    int count_;
};
