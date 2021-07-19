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
6.使用互斥锁+信号量实现线程同步，主要是eventloopthread中，每一个subreactord的主线程在调用startloop的时候还会创建一个子线程用于生成loop对象，但是loop对象是父子线程共用的，所以使用互斥锁+信号量机制，只有当子线程创建好loop对象时，父线程的startloop才会返回loop对象<br>



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
			3.loop()：间接调用epoll_wait,开始监听事件
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


## 代码运行流程
1.初始化一个main reactor：(loop对象)，创建套接字epoll_fd和eventfd，并注册可读事件(handleread)（虽然eventfd全程没有事件到来）<br>
2.根据上一步获得的reactor, 初始化server：创建套接字listenfd， 并注册可读事件handlenewconn<br>
3.server.start():初始化线程池，开始重复eventloop的构造函数(循环创建epoll_fd和eventfd)：<br>
4.main reactor.loop,开始监听所有套接字<br>



