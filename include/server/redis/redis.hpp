#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

/**
 * connect  pub_context sub_context
 * 
 * publish id message => pub_context
 * 
 * subscribe id => sub_context
 * ubsubscribe id => sub_context
 * 
 * thread
 *      redisGetReply => sub_context
 *          调用回调操作，给业务层上报 id+msg
 */


class Redis
{
public:
    Redis();
    ~Redis();

    //连接redis服务器
    bool connect();

    //向redis指定的通道publish订阅消息
    bool publish(int channel,std::string message);

    //向redis指定的通道subscribe订阅消息
    bool subscribe(int channel);

    //向redis指定的通道 unsubscribe取消订阅消息
    bool unsubscribe(int channel);

    //在独立线程中接收订阅通道中的消息
    void observe_channel_message();

    //初始化向业务层上报通道消息的回调对象
    void init_notify_handler(std::function<void(int,std::string)> fn);

private:
    //hiredis同步上下文对象，负责publish消息
    redisContext* publish_context_;

    //hiredis同步上下文对象，负责subscribe消息
    // subscribe订阅消息后，会阻塞
    redisContext* subscribe_context_;

    //回调操作，收到订阅的消息，给service层上报
    std::function<void(int,std::string)> notify_message_handler_;
};

#endif // !REDIS_H