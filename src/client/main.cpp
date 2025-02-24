#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "json.hpp"
#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

using json = nlohmann::json;

// 记录当前系统登陆的用户信息
User g_currentUser;
// 记录当前登陆用户的好友列表信息
std::vector<User> g_currentUserFriendList;
// 记录当前登陆用户的群组列表信息
std::vector<Group> g_currentUserGroupList;
// 显示当前登陆成功用户的基本信息
void showCurrentUserData();
// 控制聊天页面程序
bool isMainMenuRunning = false;

// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间 聊天信息需要添加时间信息
std::string getCurrentTime();
// 主聊天页面程序
void mainMenu(int clientfd);

// 聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << std::endl;
        exit(-1);
    }
    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        std::cerr << "socket create error" << std::endl;
        exit(-1);
    }

    // 填写client需要连接的server信息 ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    // 本地协议，IPv4
    server.sin_family = AF_INET;
    // 处理大小段存储问题
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client 和 server 连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        std::cerr << "connect server error" << std::endl;
        close(clientfd);
        exit(-1);
    }

    // main线程用于接收用户输入，负责发送数据
    for (;;)
    {
        // 显示首页面菜单 登陆、注册、退出
        std::cout << "======================" << std::endl;
        std::cout << "1. login" << std::endl;
        std::cout << "2. register" << std::endl;
        std::cout << "3. quit" << std::endl;
        std::cout << "======================" << std::endl;
        std::cout << "choice:";
        int choice = 0;
        std::cin >> choice;
        std::cin.get(); // 读掉缓冲区残留的回车

        switch (choice)
        {
        case 1: // login业务
        {
            int id = 0;
            char pwd[50] = {0};
            std::cout << "userid:";
            std::cin >> id;
            std::cin.get(); // 读掉缓冲区残留的回车
            std::cout << "userpassword:";
            std::cin.getline(pwd, 50);

            json js;
            js["msgid"] = EnMsgType::LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            std::string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                std::cerr << "send login msg error: " << request << std::endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (len == -1)
                {
                    std::cerr << "revc login response error" << std::endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>()) // 登陆失败
                    {
                        std::cerr << responsejs["errmsg"] << std::endl;
                    }
                    else // 登陆成功
                    {
                        // 记录当前用户的id和name
                        g_currentUser.setId(responsejs["id"].get<int>());
                        g_currentUser.setName(responsejs["name"]);

                        // 记录当前用户的好友列表消息
                        if (responsejs.contains("friends"))
                        {
                            std::vector<std::string> vec = responsejs["friends"];
                            for (std::string &str : vec)
                            {
                                json js = json::parse(str);
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                g_currentUserFriendList.push_back(std::move(user));
                            }
                        }

                        // 记录当前用户的好友列表信息
                        if (responsejs.contains("groups"))
                        {
                            std::vector<std::string> vec = responsejs["groups"];
                            for (std::string &groupstr : vec)
                            {
                                json grpjs = json::parse(groupstr);
                                Group group;
                                group.setId(grpjs["id"].get<int>());
                                group.setName(grpjs["groupname"]);
                                group.setDesc(grpjs["groupdesc"]);

                                std::vector<std::string> vec2 = grpjs["users"];
                                for (std::string &userstr : vec2)
                                {
                                    GroupUser user;
                                    json js = json::parse(userstr);
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    user.setRole(js["role"]);
                                    group.getUsers().push_back(std::move(user));
                                }
                                g_currentUserGroupList.push_back(std::move(group));
                            }
                        }

                        // 显示登陆用户的基本信息
                        showCurrentUserData();

                        // 显示当前用户的离线消息，个人聊天消息或者群组消息
                        if (responsejs.contains("offlineMsg"))
                        {
                            std::vector<std::string> vec = responsejs["offlineMsg"];
                            for (std::string &str : vec)
                            {
                                json js = json::parse(str);
                                if (EnMsgType::ONE_CHAT_MSG == js["msgid"].get<int>())
                                {
                                    std::cout << js["time"].get<std::string>() << " [" << js["id"] << "]"
                                              << js["name"].get<std::string>() << " said: " << js["msg"].get<std::string>() << std::endl;
                                }
                                else
                                {
                                    std::cout << "群消息[" << js["groupid"] << "]" << js["time"].get<std::string>() << " [" << js["id"] << "] " << js["name"].get<std::string>()
                                              << " said: " << js["msg"].get<std::string>() << std::endl;
                                }
                            }
                        }

                        // 登陆成功，启动接收线程负责接收数据
                        std::thread readTask(readTaskHandler, clientfd);
                        readTask.detach();

                        // 进入聊天主菜单页面
                        isMainMenuRunning = true;
                        mainMenu(clientfd);
                    }
                }
            }
            break;
        }
        case 2:
        {
            char name[50] = {0};
            char pwd[50] = {0};
            std::cout << "username:";
            std::cin.getline(name, 50);
            std::cout << "userpassword:";
            std::cin.getline(pwd, 50);

            json js;
            js["msgid"] = EnMsgType::REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            std::string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                std::cerr << "send reg msg error:" << request << std::endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    std::cerr << "recv reg response error" << std::endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>()) // 注册失败
                    {
                        std::cerr << name << " is already exist,register error!" << std::endl;
                    }
                    else // 注册成功
                    {
                        std::cout << name << " register success,userid is "
                                  << responsejs["id"] << ",do not forget it!" << std::endl;
                    }
                }
            }
            break;
        }
        case 3:
            close(clientfd);
            exit(0);
        default:
            std::cerr << "invalid input!" << std::endl;
            break;
        }
    }
}

// 显示当前登陆成功用户的基本信息
void showCurrentUserData()
{
    std::cout << "======================login user======================" << std::endl;
    std::cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << std::endl;
    std::cout << "----------------------friend list---------------------" << std::endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            std::cout << user.getId() << " " << user.getName() << " " << user.getState() << std::endl;
        }
    }
    std::cout << "----------------------group list----------------------" << std::endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            std::cout << group.getId() << " " << group.getName() << " " << group.getDesc() << std::endl;
            for (GroupUser &user : group.getUsers())
            {
                std::cout << user.getId() << " " << user.getName() << " " << user.getState()
                          << " " << user.getRole() << std::endl;
            }
        }
    }
    std::cout << "=======================================================" << std::endl;
}

// 接收线程
void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }

        // 接收ChatServer转发的数据，反序列化生成json对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (EnMsgType::ONE_CHAT_MSG == msgtype)
        {
            std::cout << js["time"].get<std::string>() << " [" << js["id"] << "]"
                      << js["name"].get<std::string>() << " said: " << js["msg"].get<std::string>() << std::endl;
            continue;
        }
        else if (EnMsgType::GROUP_CHAT_MSG == msgtype)
        {
            std::cout << "群消息[" << js["groupid"] << "]" << js["time"].get<std::string>() << " [" << js["id"] << "] " << js["name"].get<std::string>()
                      << " said: " << js["msg"].get<std::string>() << std::endl;
            continue;
        }
    }
}

// 获取系统时间 聊天信息需要添加时间信息
std::string getCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    // 通过不同精度获取相差的毫秒数
    uint64_t dis_millseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() - std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() * 1000;
    time_t tt = std::chrono::system_clock::to_time_t(now);
    auto time_tm = localtime(&tt);
    char strTime[25] = {0};
    sprintf(strTime, "%d-%02d-%02d %02d:%02d:%02d %03d", time_tm->tm_year + 1900,
            time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour,
            time_tm->tm_min, time_tm->tm_sec, (int)dis_millseconds);
    return strTime;
}

// "help" command handler
void help(int fd = 0, std::string str = "");
// "chat" command handler
void chat(int, std::string);
// "addfriend" command handler
void addfriend(int, std::string);
// "creategroup" command handler
void creategroup(int, std::string);
// "addgroup" command handler
void addgroup(int, std::string);
// "groupchat" command handler
void groupchat(int, std::string);
// "quit" command handler
void loginout(int, std::string = "");

// 系统支持的客户端命令列表
std::unordered_map<std::string, std::string> commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"loginout", "注销，格式loginout"}};

// 注册系统支持的客户端命令处理
std::unordered_map<std::string, std::function<void(int, std::string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

// 主聊天页面程序
void mainMenu(int clientfd)
{
    help();

    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
        std::cin.getline(buffer, 1024);
        std::string commandbuf(buffer);
        std::string command; // 存储命令
        int idx = commandbuf.find(":");
        if (-1 == idx)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        std::unordered_map<std::string, std::function<void(int, std::string)>>::iterator it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            std::cerr << "invalid input command!" << std::endl;
            continue;
        }

        // 调用相应命令的事件处理回调，mainMenu对修改封闭，添加新功能，不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx)); // 调用命令处理方法
    }
}

// “help" command handler
void help(int, std::string)
{
    std::cout << "show command list>>>>" << std::endl;
    for (std::pair<const std::string, std::string> &p : commandMap)
    {
        std::cout << p.first << " : " << p.second << std::endl;
    }
    std::cout << std::endl;
}

//"addfriend" command handler
void addfriend(int clientfd, std::string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = EnMsgType::ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        std::cerr << "send addfriend msg error -> " << buffer << std::endl;
    }
}

// "chat" command handler
void chat(int clientfd, std::string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        std::cerr << "chat command invalid!" << std::endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = EnMsgType::ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["time"] = getCurrentTime();
    js["toid"] = friendid;
    js["msg"] = message;
    // js["time"] = getCurrentTime();

    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send chat msg error -> " << buffer << std::endl;
    }
}

// "creategroup" command handler groupname:groupdesc
void creategroup(int clientfd, std::string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        std::cerr << "creategroup command invalid!" << std::endl;
        return;
    }

    std::string groupname = str.substr(0, idx);
    std::string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = EnMsgType::CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;

    std::string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send creategroup msg error -> " << buffer << std::endl;
    }
}

// "addgroup" command handler
void addgroup(int clientfd, std::string str)
{
    int groupid = atoi(str.c_str());

    json js;
    js["msgid"] = EnMsgType::CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;

    std::string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send addgroup msg error -> " << buffer << std::endl;
    }
}

// "groupchat" command handler groupid:message
void groupchat(int clientfd, std::string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        std::cerr << "groupchat command invalid!" << std::endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    std::string msg = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = EnMsgType::GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = msg;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send creategroup msg error -> " << buffer << std::endl;
    }
}

// "quit" command handler   groupid
void loginout(int clientfd, std::string str)
{
    json js;
    js["msgid"] = EnMsgType::LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();

    std::string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send loginout msg error -> " << buffer << std::endl;
    }
    isMainMenuRunning = false;
}
