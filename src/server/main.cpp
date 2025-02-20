#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <signal.h>

//处理服务器 ctrl+c 结束后，重置user的状态信息
void resetHandler(int)
{
    ChatService::getInstance()->reset();
    exit(0);
}


int main()
{
    signal(SIGINT,resetHandler);

    EventLoop loop;
    InetAddress addr("127.0.0.1",6000);
    ChatServer chatServer(&loop,addr,"ChatServer");

    //启动聊天室
    chatServer.start();
    //开启事件循环
    loop.loop();

    return 0;
}

