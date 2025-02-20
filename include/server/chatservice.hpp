#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <muduo/base/Logging.h>
#include <mutex>

#include "json.hpp"
#include "public.hpp"
#include "usermodel.hpp"

using json = nlohmann::json;
using namespace muduo;
using namespace muduo::net;

//表示处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json js, Timestamp time)>;

// 聊天服务器业务类
class ChatService
{
public:
    ~ChatService() = default;
    // 删除构造函数
    ChatService(const ChatService &) = delete;
    ChatService &operator=(const ChatService &) = delete;
    ChatService(const ChatService &&) = delete;
    ChatService &&operator=(const ChatService &&) = delete;
    //获取单例对象的接口函数
    static ChatService* getInstance();

    //处理登陆业务
    void login(const TcpConnectionPtr &conn, json js, Timestamp time);

    //处理注册业务
    void reg(const TcpConnectionPtr &conn, json js, Timestamp time);

    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);

private:
    ChatService();
    static ChatService chatService;

    //存储消息id和其对应的业务处理方法
    std::unordered_map<int,MsgHandler> msgHandlerMap_;

    //存储在线用户的通信连接
    std::unordered_map<int,TcpConnectionPtr> userConnMap_;
    //定义互斥锁，保证userConnMap_的线程安全
    std::mutex connMtx_;

    //数据操作类对象,注意线程安全
    UserModel userModel_;
};


#endif