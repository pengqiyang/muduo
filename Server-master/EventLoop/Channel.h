//A selectable I/O channel.
//This class doesn't own thr file descriptor
//The file descriptor could be a socket
//an eventfd, a timerfd, or a signalfd
//负责一个fd（listen_fd或accepted_fd）的IO事件的分发
#ifndef _CHANNEL_H
#define _CHANNEL_H
#include "../Encapsulate/Timer.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <sys/epoll.h>
#include <functional>
#include <sys/epoll.h>

class Channel
{
public:
    typedef std::function<void()> EventCallBack;//这是一个模板类，<>里面的void()代表函数类型是一个无参数，返回值为void的函数
    Channel(EventLoop *loop, int fd);
    int getFd();
	void setFd(int fd);
    void setHolder(std::shared_ptr<ClientRequest> holder);
    std::shared_ptr<ClientRequest> getHolder();
    void setReadHandler(EventCallBack &&cb);
    void setWriteHandler(EventCallBack &&cb);
    void setErrorHandler(EventCallBack &&cb);
    void setConnHandler(EventCallBack &&cb);
    void handleEvents();
    void handleRead();
    void handleWrite();
    void handleError(int fd, int err_num, std::string short_msg);
    void handleConn();
    void setRevents(int ev);
    void setEvents(int ev);
    int& getEvents();
    bool EqualAndUpdateLastEvents();
private:
    
    EventLoop *loop_;
    int  fd_;
    int  events_;
    int revents_;// it's the received event types of epoll used by Epoll
    std::weak_ptr<ClientRequest> holder_;
    EventCallBack readHandler_;
    EventCallBack writeHandler_;//回调函数
    EventCallBack errorHandler_;
    EventCallBack connHandler_;
};

#endif
