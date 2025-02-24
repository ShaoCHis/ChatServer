#include "groupmodel.hpp"
#include <unordered_map>

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert allgroup(groupname,groupdesc) values('%s','%s')", group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 执行更新操作
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

// 加入群组
void GroupModel::addGroup(int userid, int groupid, std::string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert groupuser(groupid,userid,grouprole) values(%d,%d,'%s')", groupid, userid, role.c_str());

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
    char sql[1024] = {0};
    sprintf(sql, "select m.*\
                from \
                    (select a.*,b.groupname,b.groupdesc \
                    from \
                        (select a.id,a.name,a.state,b.groupid,b.grouprole \
                        from user a inner join groupuser b on a.id=b.userid) a \
                        inner join allgroup b on a.groupid=b.id) m \
                    inner join \
                    (select groupid \
                    from groupuser \
                    where userid=%d) n on m.groupid=n.groupid",
            userid);

    std::unordered_map<int, std::vector<GroupUser>> groups;
    std::vector<Group> result;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                int groupid = atoi(row[3]);
                // 添加GroupUser信息
                if (groups.find(groupid) != groups.end())
                {
                    GroupUser groupUser(atoi(row[0]), row[1], row[2], row[4]);
                    groups[groupid].push_back(std::move(groupUser));
                }
                else
                {
                    groups.insert({groupid, {std::move(GroupUser(atoi(row[0]), row[1], row[2], row[4]))}});
                    // 添加Group信息
                    result.push_back(std::move(Group(groupid, row[5], row[6])));
                }
            }
            mysql_free_result(res);
        }
    }
    // std::vector<Group> result;
    for (Group &item : result)
    {
        item.setUsers(std::move(groups[item.getId()]));
    }
    return result;
}

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
