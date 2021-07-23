## 项目介绍：
linux回射服务器<br>
1.采用multiple reactors模型：<br>
main reactor只负责listenfd的事件监听，并将新连接以Round Robin的方式分发给sub reactor，由sub reacotr来负责对应套接字的监听事件<br>
2.采用epoll水平触发的IO多路复用技术<br>


## 关键技术：
1.使用Epoll水平触发的IO多路复用技术（因为程序并发数小，所以没有用ET， 相比于LT，ET模式在很大程度上降底了同一个epoll事件被重复触发的次数，因此效率要比LT模式高）<br>
2.使用多线程充分利用多核CPU，并使用线程池避免线程频繁创建销毁的开销<br>
3.为减少内存泄漏的可能，使用智能指针，shared_ptr（自动管理内存，当引用计数为0，自动析构对象）<br>
4.使用eventfd实现了线程的异步唤醒(其目的主要是为了解决线程安全问题，将读写操作放入对应的IO线程中，但是我的项目中没有用到，一般是buffer的读写需要用到)<br>
5.主线程只负责accept请求，并将新连接的连接以Round Robin的方式分发给其它IO线程(兼计算线程)<br>
6.使用互斥锁+条件变量实现线程同步，例如是eventloopthread中，每一个subreactord的主线程在调用startloop的时候还会创建一个子线程用于生成loop对象，但是loop对象是父子线程共用的，所以使用互斥锁+条件量机制，只有当子线程创建好loop对象时，父线程的startloop才会返回loop对象<br>
还有就是eventloop中的inqueueloop()和doPendingFunctors()，因为设计到对vector<functors>的多线程操作，增加读取操作，而stl不是线程安全的，所以要互斥锁+条件变量


## 项目模块介绍：
该模块参照陈硕的muduo和  git clone https://github.com/StormrageZ/WEB_SERVER.git ，主要是基于对象，改写成简单的回射服务器（没有考虑buffer设计，定时器，log记录这些）<br>
* channel：<br>
	即对文件描述符行为的一种封装<br>
	**成员变量**：<br>
		ev(关注事件类型)；readhandler_, writehandle_等各种事件的回调函数<br>
	**函数**：<br>
		1.handlevent():根据不同的事件类型，events赋值为不同的事件，events即epoll_event.events<br>
	
* eventloop:<br>
	**成员变量**：<br>
	eventfd(唤醒套接字)，poller, eventfdchannel, mutex, pendingfunctors<br>
	**函数功能**：<br>
			1.removefrompoller, updatepoller等: 对epoll_ctl增删改查功能封装<br>
			2.构造函数：设置eventfd的关注事件类型，以及注册事件到来的回调函数， 将eventfdchannel绑定到epoll_fd,<br>
			3.loop()：间接调用epoll_wait,开始监听事件，返回一个用vector封装的channel指针
			4.还有runinloop:runinloop->queuinloop->wakeup()->handlread()->dopendinginfactor()

* eventloopthread:<br>
	**成员变量**：<br>
	loop, 条件变量, 互斥锁<br>
	**函数功能：**<br>	
			1.startloop():返回创建好的loop对象, 并且开始loop.loop<br>	
			
* eventloopthreadpool:<br>
	**成员变量**：<br>	
	baseloop(即创建eventloopthreadpool的loop对象)， vector<loops>(即vector类型的loop成员)<br>	
	**函数功能**：<br>	
			1.start():调用eventloopthread的startloop函数<br>	
			2.getnextloop():返回下一个loop对象<br>	
* epollpoller:<br>
	**成员变量**：<br>
	epoll_fd(不要忘了，要想使用epoll必须先创建一个epoll_fd)<br>
	**函数功能**：<br>
	epoll_mod, epoll_add,epoll_delet, poll等，都是对epoll系统调用的封装<br>
* server:<br>
	**成员变量**：<br>
	eventloop, listenfd, listenchannel, threadnum, eventloopthreadpool<br>
	**函数功能**：<br>
			构造函数：负责listenfd, listenchannel初始化<br>
			start():启动线程池，使得每个线程池开始loop；设置listenfd的关注事件，以及注册事件到来的回调函数，将listenchannel绑定到epoll_fd,即间接调用epoll_ctl函数的add功能<br>
			handlenewconn():即连接事件到来的回调函数，Round Robin的方式给新连接分配一个IO线程<br>
			
	
* connection:<br>
	**成员变量**:<br>
	connfd, connchannel, port, addr, loop<br>
	**函数功能**：<br>
	可读事件的回调函数（即回显流程）<br>
* util:<br>
	**函数功能**：<br>
	对read, write, 绑定套接字, 套接字阻塞， 端口复用等一些列属性设置的封装。<br>

* mutexlock	,condition:<br>
	采用RAII机制对条件变量和互斥锁进行封装<br>
	mutexlock：包含MutexLockGuard和mutexlock两个类<br>
	**mutexlock类**：构造函数:pthread_mutex_init ； 析构函数 pthread_mutex_destroy ；lock函数：pthread_mutex_lock  ；unlock函数：pthread_mux_unlock<br>
	**mutexlockguard类**：构造函数：调用mutex对象构造函数，调用mutex对象lock函数； 析构函数：调用mutex对象unlock函数<br>
	**condtion类**：构造函数：pthread_cond_init；  析构函数:pthread_cond_destroy； wait函数：pthread_cond_wait,注意这里使用其第二个参数mutex变量，意思是如果不满足则unlock； signal函数：pthread_cond_signal<br>
		
		
		
## 代码运行流程
1.初始化一个main reactor：(loop对象)，创建套接字epoll_fd和eventfd，并注册可读事件(handleread)（虽然eventfd全程没有事件到来）<br>
2.根据上一步获得的reactor, 初始化server：创建套接字listenfd， 并注册可读事件handlenewconn<br>
3.server.start():初始化线程池，开始重复eventloop的构造函数(循环创建epoll_fd和eventfd)：<br>
4.main reactor.loop,开始监听所有套接字<br>


## 开发项目时遇到的问题
1.**线程安全**（当发生竞态条件（当两个线程竞争同一资源时，如果对资源的访问顺序敏感时），要注意线程安全），具体分为两个方面(1)**如何实现对共享变量的同步性操作**  (2)**如何避免悬挂指针** <br>
其实就是第一种，一个线程调用deleta A 另一个线程调用 A->update,A的析构函数中使用了互斥锁尽管第一个线程先拿到锁，执行A的析构函数，但是析构函数的执行<br>
会将互斥锁给释放掉，那另一个线程有可能会一直阻塞，也有可能不会阻塞但是因为A对象已经不存在，那么再对A对象进行操作可能会引发core dump内存错误访问等，<br>
所以这个时候互斥锁就没有用，因此引入shared_ptr解决问题，shared_ptr可以控制对象的生命周期，当引用计数为0时，对象才被析构这样就解决了野指针问题；<br>
除此之外由于对象的析构交给智能指针自动管理，因此就不会存在忘记delete的情景；**对象构造**要做到线程安全不要在构造函数中注册回调函数；<br>
不要在构造函数中把this指针传给跨线程的对象，因为构造过程一些对象还没有创建好,如果把指针暴露给其他线程很危险（有点类似构造函数中的虚函数）<br>；
**析构函数**中线程安全仅靠互斥锁不行的<br>


2.**线程同步三原则**：(1)其实最好的并不是借助同步原语或者一些高级并发架构，而是从根本上减少这种共享，并发的现象，一个对象能不暴露给别的线程就不要暴露，比如<br>
muduo的就是尽量把左右操作放在一个线程中做，(2)借助高级并发编程构建，例如countDownLatch,我们项目中的mutexlock和condtion就是一种countDownLatch, （3）迫不得已使用<br>
底层同步原语，一般不考虑读写锁（当read>write线程数，会引发写饥饿）、信号量（代码麻烦，容易出错），而是互斥锁+条件变量


	
3.	用到shared_Ptr的类以及原因<br>
(1)比如eventloopthreadpool中的vector<shared_ptr<EventLoopThread>> threads_，因为eventloopthread属于主线程，会创建eventloopthread对象， 但是这个对象被其他子线程
使用，所以就要用智能指针，避免野指针

4.大量使用右值引用和std(move)实现移动拷贝
