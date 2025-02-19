#include "chatservice.hpp"



ChatService *ChatService::getInstance()
{
    static ChatService chatService;
    return &chatService;
}

// 注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    msgHandlerMap_.insert({EnMsgType::LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});
    msgHandlerMap_.insert({EnMsgType::REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});
    //提供一个默认的处理器
}

//获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    //记录错误日志，msgid没有对应的事件处理回调
    if(msgHandlerMap_.find(msgid)==msgHandlerMap_.end())
    {
        //返回一个默认的处理器，空操作
        return [&](const TcpConnectionPtr &conn, json js, Timestamp time){
            //muduo库日志系统打印
            LOG_ERROR << "msgid:" << msgid << " can not find handler!";
        };
    }
    else
    {
        return msgHandlerMap_[msgid];
    }
}

//处理登陆业务  ORM  业务层操作的都是对象   DAO 
void ChatService::login(const TcpConnectionPtr &conn, json js, Timestamp time)
{
    LOG_INFO << "do login service!!";
}

//处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json js, Timestamp time)
{
    LOG_INFO << "do reg service!!!";
}
