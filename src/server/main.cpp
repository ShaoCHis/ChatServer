#include "chatserver.hpp"
#include <iostream>

int main()
{
    EventLoop loop;
    InetAddress addr("127.0.0.1",6000);
    ChatServer chatServer(&loop,addr,"ChatServer");

    //启动聊天室
    chatServer.start();
    //开启事件循环
    loop.loop();

    return 0;
}

