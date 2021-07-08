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
    loop_(loop),//�¼�ѭ��
    addr_(addr),//��ַ
    port_(port),//�˿�
    connFd_(connfd),//�����׽���
    connChannel_(new Channel(loop_, connFd_))//����ͨ��
{
	cout<<"��������: "<<Connection::addr_<<" "<<Connection::port_<<"������"<<endl;
    connChannel_->setEvents(EPOLLIN | EPOLLET);//���ø���Ȥ���¼� ����д�¼�
    connChannel_->setReadHandler(bind(&Connection::messagecallback, this));//��ȡ������Ϣ
    loop_->addToPoller(connChannel_, 0);
}


void Connection::messagecallback()
{
	memset(buf,0,sizeof(buf));
	int len = recv(connFd_, buf, sizeof(buf),0);
    printf("���Կͻ������ݣ�%s\n", buf);
}
