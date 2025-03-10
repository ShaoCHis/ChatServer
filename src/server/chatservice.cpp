#include "chatservice.hpp"
#include <iostream>

ChatService *ChatService::getInstance()
{
    static ChatService chatService;
    return &chatService;
}

// 注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    // 用户基本业务管理相关事件处理回调注册
    msgHandlerMap_.insert({EnMsgType::LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    msgHandlerMap_.insert({EnMsgType::REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    msgHandlerMap_.insert({EnMsgType::ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    msgHandlerMap_.insert({EnMsgType::ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    msgHandlerMap_.insert({EnMsgType::LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});

    // 群组业务管理相关事件处理回调注册
    msgHandlerMap_.insert({EnMsgType::CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    msgHandlerMap_.insert({EnMsgType::ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    msgHandlerMap_.insert({EnMsgType::GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 连接redis服务器
    if (redis_.connect())
    {
        // 设置上报消息的回调
        redis_.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        std::lock_guard<std::mutex> lock(connMtx_);
        for (std::unordered_map<int, TcpConnectionPtr>::iterator item = userConnMap_.begin(); item != userConnMap_.end(); item++)
        {
            if (item->second == conn)
            {
                // 从map表删除用户的连接信息
                user.setId(item->first);
                userConnMap_.erase(item);
                break;
            }
        }
    }

    // id用户下线，在redis中取消订阅通道
    redis_.unsubscribe(user.getId());

    if (user.getId() != -1)
    {
        // 更新用户的额状态信息
        user.setState("offline");
        userModel_.updateState(user);
    }
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    // 把online状态的用户，设置成offline
    userModel_.resetState();
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
        if (user.getState() == "online")
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this id is already online,please change id!!!";
            conn->send(response.dump());
        }
        else
        {
            {
                // 登陆成功，记录用户连接信息
                std::lock_guard<std::mutex> lock(connMtx_);
                userConnMap_.emplace(user.getId(), conn);
            }

            // id用户登录成功后，向redis订阅channel(id)
            redis_.subscribe(user.getId());

            // 登陆成功，更新用户状态信息 state offline => online
            user.setState("online");
            userModel_.updateState(user);
            LOG_INFO << "Login Success!!";
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            // 查询该用户是否有离线消息
            std::vector<std::string> vec = offlineMsgModel_.query(user.getId());
            if (!vec.empty())
            {
                response["offlineMsg"] = vec;
                // 读取该用户的离线消息后，把该用户的所有离线消息删除掉
                offlineMsgModel_.remove(user.getId());
            }

            // 查询该用户的好友信息并返回
            std::vector<User> friends = friendModel_.query(user.getId());
            if (!friends.empty())
            {
                std::vector<std::string> userVec;
                for (User user : friends)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    userVec.emplace_back(js.dump());
                }
                response["friends"] = userVec;
            }
            // 拼接群聊名称进行返回显示
            std::vector<Group> groups = groupModel_.queryGroups(user.getId());
            auto groupsInfo = [&groups]() -> std::vector<std::string>
            {
                // group:[{groupid:[xxxx,xxxx,xxx,xxx]}]
                std::vector<std::string> groupV;
                for (Group &group : groups)
                {
                    json grpjs;
                    grpjs["id"] = group.getId();
                    grpjs["groupname"] = group.getName();
                    grpjs["groupdesc"] = group.getDesc();
                    std::vector<std::string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjs["users"] = userV;
                    groupV.push_back(grpjs.dump());
                }
                return groupV;
            };
            response["groups"] = groupsInfo();
            std::cout << response.dump().size() << std::endl;
            std::cout << response.dump() << std::endl;
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
        response["errmsg"] = "the userid or password is wrong!";
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

// 处理一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json js, Timestamp time)
{
    int toid = js["toid"].get<int>();
    bool userState = false;
    {
        std::lock_guard<std::mutex> lock(connMtx_);
        std::unordered_map<int, TcpConnectionPtr>::iterator item = userConnMap_.find(toid);
        if (item != userConnMap_.end())
        {
            // toid在线，转发消息    服务器主动推送消息给toid用户
            // 在同一台服务器上登录直接转发
            item->second->send(js.dump());
            return;
        }
    }

    // 查询toid用户是否在其他服务器上登录
    User user = userModel_.query(toid);
    if (user.getState() == "online")
    {
        // toid用户在其他服务器上登录
        redis_.publish(toid, js.dump());
        return;
    }

    // toid不在线，存储离线消息
    offlineMsgModel_.insert(toid, js.dump());
}

// 添加好友业务  msg id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendId = js["friendid"].get<int>();

    // 存储好友信息
    friendModel_.insert(userid, friendId);
}

// 创建群组业务  msg groupname groupdesc
void ChatService::createGroup(const TcpConnectionPtr &conn, json js, Timestamp time)
{
    int userid = js["id"].get<int>();
    Group group;
    group.setName(js["groupname"]);
    group.setDesc(js["groupdesc"]);
    // 存储新创建的群组信息
    if (groupModel_.createGroup(group))
    {
        // 存储群组创建人信息
        groupModel_.addGroup(userid, group.getId(), "creator");
    }
}

// 添加群组业务  msg,int userid,int groupid,std::string role
void ChatService::addGroup(const TcpConnectionPtr &conn, json js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    groupModel_.addGroup(userid, groupid, "normal");
}

// 群聊业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json js, Timestamp time)
{
    int groupId = js["groupid"].get<int>();
    int userId = js["id"].get<int>();
    std::vector<int> usersId = groupModel_.queryGroupUsers(userId, groupId);

    for (int id : usersId)
    {
        std::lock_guard<std::mutex> lock(connMtx_);
        // 用户在线，发送消息
        if (userConnMap_.find(id) != userConnMap_.end())
        {
            userConnMap_[id]->send(js.dump());
        }
        else
        {
            // 查询toid用户是否在其他服务器上登录
            User user = userModel_.query(id);
            if (user.getState() == "online")
            {
                // toid用户在其他服务器上登录
                redis_.publish(id, js.dump());
            }
            else
            {
                // 用户不在线，插入离线消息
                offlineMsgModel_.insert(id, js.dump());
            }
        }
    }
}

void ChatService::loginout(const TcpConnectionPtr &conn, json js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        std::lock_guard<std::mutex> lock(connMtx_);
        auto it = userConnMap_.find(userid);
        if (it != userConnMap_.end())
        {
            userConnMap_.erase(it);
        }
    }
    // 更新用户的额状态信息
    userModel_.updateState(std::move(User(userid, "", "", "offline")));

    // id用户登出成功后，在redis中取消订阅通道
    redis_.unsubscribe(userid);
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, std::string msg)
{
    {
        std::lock_guard<std::mutex> lock(connMtx_);
        auto item = userConnMap_.find(userid);
        if(item!=userConnMap_.end()){
            item->second->send(msg);
            return;
        }
    }
    offlineMsgModel_.insert(userid,msg);
}
