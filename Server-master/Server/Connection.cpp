#include "../Server/Connection.h"
#include "../Log/Logging.h"
#include "../Encapsulate/Util.h"
#include <functional>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>
#include <string>  
#include <iostream>  
#define BUFFER_SIZE 1024
using namespace std;
char buf[BUFFER_SIZE];
Connection::Connection(EventLoop *loop, string addr, int port, int connfd):
    loop_(loop),//事件循环
    addr_(addr),//地址
    port_(port),//端口
    connFd_(connfd),//链接套接字
    connChannel_(new Channel(loop_, connFd_))//连接通道
{
	cout<<"接受来自: "<<Connection::addr_<<" "<<Connection::port_<<"的连接"<<endl;
    connChannel_->setEvents(EPOLLIN | EPOLLET);//设置感兴趣的事件 读和写事件
    connChannel_->setReadHandler(bind(&Connection::messagecallback, this));//读取连接信息
    loop_->addToPoller(connChannel_, 0);
}


void Connection::messagecallback()
{
	memset(buf,0,sizeof(buf));
	int len = recv(connFd_, buf, sizeof(buf),0);
    printf("来自客户端数据：%s\n", buf);
}
