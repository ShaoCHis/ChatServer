#include "chatservice.hpp"

ChatService *ChatService::getInstance()
{
    static ChatService chatService;
    return &chatService;
}

// 注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    msgHandlerMap_.insert({EnMsgType::LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    msgHandlerMap_.insert({EnMsgType::REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    // 提供一个默认的处理器
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    if (msgHandlerMap_.find(msgid) == msgHandlerMap_.end())
    {
        // 返回一个默认的处理器，空操作
        return [&](const TcpConnectionPtr &conn, json js, Timestamp time)
        {
            // muduo库日志系统打印
            LOG_ERROR << "msgid:" << msgid << " can not find handler!";
        };
    }
    else
    {
        return msgHandlerMap_[msgid];
    }
}

// 处理登陆业务  ORM  业务层操作的都是对象   DAO
// id pwd
void ChatService::login(const TcpConnectionPtr &conn, json js, Timestamp time)
{
    int id = js["id"];
    std::string pwd = js["password"];
    User user = userModel_.query(id);
    if (user.getId() == id && user.getPassword() == pwd)
    {
        if (user.getState() == "onlie")
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "该账号已经登陆，请重新输入新账号";
            conn->send(response.dump());
        }
        else
        {
            // 登陆成功，更新用户状态信息 state offline => online
            user.setState("online");
            userModel_.updateState(user);
            LOG_INFO << "Login Success!!";
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            conn->send(response.dump());
        }
    }
    else
    {
        // 该用户不存在 或者 用户存在但是密码错误，登陆失败
        LOG_ERROR << "Wrong Password or user not exists!";
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户名或者密码错误！";
        conn->send(response.dump());
    }
}

// 处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json js, Timestamp time)
{
    std::string name = js["name"];
    std::string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPassword(pwd);
    bool state = userModel_.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        response["error msg"] = "Register failed!!Please try again later!";
        conn->send(response.dump());
    }
}
