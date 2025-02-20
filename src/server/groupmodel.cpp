#include "groupmodel.hpp"

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert allgroup(groupname,groupdesc) values('%s','%s')", group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        // 执行更新操作
        group.setId(mysql_insert_id(mysql.getConnection()));
        return mysql.update(sql);
    }
    return false;
}

// 加入群组
void GroupModel::addGroup(int userid, int groupid, std::string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert groupuser(groupid,userid,grouprole) values(%d,%d,'%s')", userid, groupid, role.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        // 执行更新操作
        mysql.update(sql);
    }
}

// 查询用户所在群组信息
std::vector<Group> GroupModel::queryGroups(int userid)
{
    std::vector<Group> groups;
    char sql[1024] = {0};
    return groups;
}

//BUG
// 根据指定的groupid查询群组用户id列表，除userid自己，主要用户群聊业务给群组其他成员群发消息
std::vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    std::vector<int> usersId;
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid=%d and userid!=%d", groupid, userid);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {       
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                usersId.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return usersId;
}
