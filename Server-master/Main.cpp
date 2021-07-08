#include "Encapsulate/Util.h"
#include "EventLoop/EventLoop.h"
#include "Server/Server.h"
#include "Log/Logging.h"
#include <getopt.h>
#include <string>
/*
void onMessage(const muduo::net::TcpConnectionPtr& conn,
                           muduo::net::Buffer* buf,
                           muduo::Timestamp time)//消息达到的关注事件
{
  muduo::string msg(buf->retrieveAllAsString());
  LOG_INFO << conn->name() << " echo " << msg.size() << " bytes, "
           << "data received at " << time.toString();
  conn->send(msg);
}
*/

int main(int argc, char *argv[])
{
    int threadNum = 4;
    int port = 65522;

    cout<<"初始化主线程\n";
    EventLoop Main;//非new定义对象，在栈中分配存储空间
   cout<<"初始化子线程\n";
    Server HTTPServer(&Main, threadNum, port);
    cout<<"开启线程池\n";
    HTTPServer.start();
    cout<<"开启主线程\n";
    Main.loop();
    return 0;
}
