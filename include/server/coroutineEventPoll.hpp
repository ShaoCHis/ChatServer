#pragma once
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <sys/epoll.h>
#include <fcntl.h>
#include <queue>


#include <boost/context/execution_context.hpp>


#include <memory>
#include <functional>
#include <vector>

#define COROUTINE_STACK_SIZE 2048
#define COROUTINE_MAX_COUNT 128
#define PORT 8000

namespace ctx = boost::context;

using Task = std::function<void(int,int)>;

class Future;
class CoroutinePool;
class Coroutine;
struct CoroutinePoolCtx;

enum class CoroutineState{
    RUNNING,        //协程正在运行
    IDEL,           //协程空闲，但是有任务，只是不具备CPU条件
    UNSED           //协程无任务
};

class Coroutine
{
public:

    //设置协程执行的任务
    void setTask(Task);

    int getNumber();

    //协程继续执行
    void resum(int,int);

    Coroutine(int number);

private:
    Task task_;
    int number_;

    //协程上下文
    ctx::execution_context<int,int> coroutineCtx_;
};


class CoroutinePool{
public:

    //单例模式，删除构造函数
    CoroutinePool(const CoroutinePool &) = delete;
    CoroutinePool(const CoroutinePool &&) = delete;

    CoroutinePool& operator=(const CoroutinePool&) = delete;
    CoroutinePool& operator=(const CoroutinePool&&) = delete;

    //获取单例
    static CoroutinePool* getInstance();

    //析构函数
    ~CoroutinePool();

    //协程池启动函数
    void start(uint32_t coroutineCnt);

    //重启某个协程
    void resum(std::shared_ptr<Coroutine>,int,int);

    //创建连接
    void r_accept(int,int);

    //读任务
    void r_recv(int,int);

    //写任务
    void r_send(int,int);

private:
    //构造函数
    CoroutinePool() = default;
    //每个线程初始创建的协程数量，空闲状态的协程数量
    uint32_t coroutineCnt_;
    //最大协程数量
    uint32_t coroutineMaxCnt_ = COROUTINE_MAX_COUNT;
    //采用共享指针来存储
    std::queue<Coroutine*> coroutines_;

    //epoll实例
    int epoll_fd_;
    //listen fd
    int listenfd_;

    //epoll_events
    epoll_event events_[1024] = {0};

    //ip+port
    uint32_t port_ = PORT;
};


