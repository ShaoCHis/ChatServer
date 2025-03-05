#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <signal.h>
#include "coroutineEventPoll.hpp"

//处理服务器 ctrl+c 结束后，重置user的状态信息
void resetHandler(int)
{
    ChatService::getInstance()->reset();
    
    exit(0);
}


int main(int argc,char **argv)
{
    if(argc<3)
    {
        std::cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << std::endl;
    }

    //解析通过命令行参数传递的ip 和 port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    signal(SIGINT,resetHandler);

    EventLoop loop;
    InetAddress addr(ip,port);
    ChatServer chatServer(&loop,addr,"ChatServer");

    //启动聊天室
    chatServer.start();
    //开启事件循环
    loop.loop();

    return 0;
}

