#include "offlinemessagemodel.hpp"
#include "db.h"
#include <muduo/base/Logging.h>

// 存储用户的离线消息
void OfflineMsgModel::insert(int userid, std::string msg)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values(%d,'%s')", userid, msg.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 删除用户的离线消息
void OfflineMsgModel::remove(int userid)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid =%d", userid);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询用户的离线消息
std::vector<std::string> OfflineMsgModel::query(int userid)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid=%d", userid);

    MySQL mysql;
    std::vector<std::string> vec;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        vec.reserve(res->data->rows);
        LOG_INFO << "res data counts:" << res->data->rows << " ,vector size:" << vec.size();
        if (res != nullptr)
        {
            // 把userid用户的所有离线消息放入vec中返回
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.emplace_back(row[0]);
            }
            mysql_free_result(res);
        }
    }
    return vec;
}
