/**
 * muduo网络库给用户提供了两个主要的类
 * TcpServer ：用于编写服务器程序的
 * TcpClient ：用于编写客户端程序的
 *
 * epoll + 线程池
 * 好处是：能够把网络I/O的代码和业务代码区分开
 *                          用户的连接和断开    用户的可读写事件
 */

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>
#include <time.h>

// 基于muduo网络库开发服务器程序
/**
 * 1.组合TcpServer对象
 * 2.创建EventLoop事件循环对象的指针
 * 3.明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
 * 4.在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写事件的回调函数
 * 5.设置合适的服务端线程数量，muduo库会自己分配I/O线程和worker线程
 */
class ChatServer
{
public:
    ChatServer(muduo::net::EventLoop *loop,               // 事件循环
               const muduo::net::InetAddress &listenAddr, // IP+Port
               const std::string &nameArg)                     // 服务器的名字
        : server_(loop, listenAddr, nameArg), loop_(loop)
    {
        ////////回调：：：模块的解耦
        // 给服务器注册用户连接的创建和断开回调
        /**
         * ConnectionCallback为回调函数指针
         * void setConnectionCallback(const ConnectionCallback& cb)
         */
        server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, std::placeholders::_1));

        // 给服务器注册用户读写事件回调
        /**
         * MessageCallback为回调函数指针
         * void setMessageCallback(const MessageCallback& cb)
         */
        server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        // 设置服务器端的线程数量 1个I/O线程  3个worker线程
        server_.setThreadNum(4);
    }

    // 开启事件循环
    void start()
    {
        server_.start();
    }

private:
    // 专门处理用户的连接创建与断开。epoll。listenfd  accept
    void onConnection(const muduo::net::TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            std::cout << conn->peerAddress().toIpPort() << "->" << conn->localAddress().toIpPort() << " state:online!" << std::endl;
        }
        else
        {
            std::cout << conn->peerAddress().toIpPort() << "->" << conn->localAddress().toIpPort() << " state:offline!" << std::endl;
            conn->shutdown();   //close(fd)
            //  loop_->quit();  回收epoll资源
        }
    }

    // 专门处理用户的读写事件
    void onMessage(const muduo::net::TcpConnectionPtr &conn, // 连接
                   muduo::net::Buffer *buffer,                              // 缓冲区
                   muduo::Timestamp time)                           // 接收到数据的时间信息
    {
        std::string buf = buffer->retrieveAllAsString();
        std::cout << "recv data:" << buf << " time:" << time.toString() << std::endl; 
        conn->send(buf);
    }
    muduo::net::TcpServer server_; // #1
    muduo::net::EventLoop *loop_;  // #2
};

int main()
{
    muduo::net::EventLoop loop; //epoll
    muduo::net::InetAddress addr("127.0.0.1",6000);
    ChatServer chatServer(&loop,addr,"ChatServer");

    chatServer.start(); //listenfd  epoll_ctl=>epoll
    loop.loop();    //epoll_wait以阻塞方式等待新用户连接，已连接用户的读写事件等
    return 0;
}

