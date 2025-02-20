#include "friendmodel.hpp"

// 添加好友关系
void FriendModel::addFriend(int userId, int friendId)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    char sql2[1024] = {0};
    sprintf(sql, "insert into friend values(%d,%d)", userId, friendId);
    sprintf(sql2, "insert into friend values(%d,%d)", friendId, userId);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
        mysql.update(sql2);
    }

    
}

// 返回用户好友列表  friendId
std::vector<User> FriendModel::query(int userId)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    //冗余存储一份
    sprintf(sql, "select a.id,a.name,a.state\
                from user a inner join friend b\
                on b.friendid=a.id \
                where b.userid = %d", userId);

    MySQL mysql;
    std::vector<User> vec;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            // 把userid用户的所有离线消息放入vec中返回
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.emplace_back(User(atoi(row[0]),row[1],"",row[2]));
            }
            mysql_free_result(res);
        }
    }
    return vec;
}
