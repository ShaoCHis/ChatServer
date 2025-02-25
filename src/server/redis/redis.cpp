#include "redis.hpp"
#include <iostream>

Redis::Redis() : publish_context_(nullptr), subscribe_context_(nullptr)
{
}

Redis::~Redis()
{
    if (publish_context_ != nullptr)
    {
        redisFree(publish_context_);
    }

    if (subscribe_context_ != nullptr)
    {
        redisFree(subscribe_context_);
    }
}

// 连接redis服务器
bool Redis::connect()
{
    // 负责publish发布消息的上下文连接
    publish_context_ = redisConnect("127.0.0.1", 6379);
    if (nullptr == publish_context_)
    {
        std::cerr << "connect redis failed!" << std::endl;
        return false;
    }

    // 负责subscribe订阅消息的上下文连接
    subscribe_context_ = redisConnect("127.0.0.1", 6379);
    if (nullptr == subscribe_context_)
    {
        std::cerr << "connect redis failed!" << std::endl;
        return false;
    }

    //在单独的线程中，监听通道上的事件，有消息给业务层上报
    std::thread t([&](){
        observe_channel_message();
    });
    t.detach();

    std::cout << "connect redis-server success!" << std::endl;
    return true;
}

// 向redis指定的通道publish订阅消息
bool Redis::publish(int channel, std::string message)
{
    //PUBLISH channelid message
    /***
     * redisCommand工作原理
     * 先将要发送的命令通过redisAppendCommand缓存到本地
     * 然后通过redisBufferWrite发送到redis上
     * 然后通过redisGetReply以阻塞的方式来等待这个命令的执行结果
     */
    redisReply *reply = (redisReply *)redisCommand(publish_context_,"PUBLISH %d %s",channel,message.c_str());
    if(nullptr==reply)
    {
        std::cerr << "publish command failed!" << std::endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
    //SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接收通道消息
    //通道消息的接收专门在observe_channel_message函数中的独立线程中进行
    //只负责发送命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢占响应资源
    /**
     * publish是立马返回，subscribe是阻塞等待
     * 所以这两个的命令区别在于这儿，得分开实现相关代码
     */
    if(REDIS_ERR==redisAppendCommand(this->subscribe_context_,"SUBSCRIBE %d",channel))
    {
        std::cerr << "subscibe command failed!" << std::endl;
        return false;
    }
    //redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕(done被置为1)
    int done = 0;
    while(!done)
    {
        if(REDIS_ERR==redisBufferWrite(this->subscribe_context_,&done))
        {
            std::cerr << "subscribe command failed!" << std::endl;
            return false;
        }
    }
    //redisGetReply()
    return true;
}

// 向redis指定的通道 unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
    //unsubscribe与subscribe同理
    if(REDIS_ERR==redisAppendCommand(this->subscribe_context_,"UNSUBSCRIBE %d",channel))
    {
        std::cerr << "unsubscibe command failed!" << std::endl;
        return false;
    }
    //redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕(done被置为1)
    int done = 0;
    while(!done)
    {
        if(REDIS_ERR==redisBufferWrite(this->subscribe_context_,&done))
        {
            std::cerr << "unsubscribe command failed!" << std::endl;
            return false;
        }
    }
    //redisGetReply()
    return true;
}

// 在独立线程中接收订阅通道中的消息
void Redis::observe_channel_message()
{
    redisReply *reply = nullptr;
    //redisGetReply是阻塞等待通道上的消息返回
    while(REDIS_OK==redisGetReply(this->subscribe_context_,(void **)&reply))
    {
        //订阅收到的消息是一个带三元素的数组
        //"type"
        //"channelid"
        //"msg"
        if(reply!=nullptr && reply->element[2] != nullptr && reply->element[2]->str!=nullptr)
        {
            //给业务层上报通道上发生的消息
            notify_message_handler_(atoi(reply->element[1]->str),reply->element[2]->str);
        }

        freeReplyObject(reply);
    }
}

// 初始化向业务层上报通道消息的回调对象
void Redis::init_notify_handler(std::function<void(int, std::string)> fn)
{
    this->notify_message_handler_ = fn;
}
