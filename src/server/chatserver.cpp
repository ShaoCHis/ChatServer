#include "chatserver.hpp"
#include "chatservice.hpp"
#include <chrono>
#include <thread>
#include <string>

// 初始化聊天服务器对象
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : server_(loop, listenAddr, nameArg), loop_(loop)
{
    // 注册链接回调
    server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, std::placeholders::_1));

    // 注册消息回调
    server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 设置线程数量 如果是4的话，那么分别对应1个I/O线程  3个worker线程
    server_.setThreadNum(std::thread::hardware_concurrency());
}

// 启动服务
void ChatServer::start()
{
    server_.start();
}

// 上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    //客户端断开连接，释放socket资源
    if(!conn->connected())
    {
        ChatService::getInstance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    std::string buf = buffer->retrieveAllAsString(); 
    //数据的反序列化，数据的解码
    json js = json::parse(buf);
    //达到的目的：完全解耦网络模块的代码和业务模块的代码
    //通过js["msgid"] 绑定回调操作，获取=》业务handler =》 conn js time
    MsgHandler msghdr = ChatService::getInstance()->getHandler((js["msgid"]).get<int>());
    //回调消息绑定好的处理器，来执行相应的业务处理
    msghdr(conn,js,time);
}
