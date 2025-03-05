#include "test.pb.h"
#include <iostream>
#include <string>

int main()
{
    fixbug::LoginResponse rsp;
    //对于嵌套对象，提供了一个mutable指针，用来修改对象的值
    fixbug::ResultCode *rc = rsp.mutable_result();
    rc->set_errcode(1);
    rc->set_errmsg("登录处理失败了");

    fixbug::GetFriendsListResponse gRsp;
    fixbug::ResultCode *gRc = gRsp.mutable_result();
    gRc -> set_errcode(0);

    //通过返回列表中的一个指针给用户进行值的修改
    fixbug::User *user1 = gRsp.add_friendslist();
    user1->set_name("zhang san");
    user1->set_age(20);
    user1->set_sex(fixbug::User::MAN);

    fixbug::User *user2 = gRsp.add_friendslist();
    user2->set_name("zhang san");
    user2->set_age(20);
    user2->set_sex(fixbug::User::WOMAN);
    std::cout << gRsp.friendslist_size() << std::endl;
};



int main1()
{
    //封装了login请求对象的数据
    fixbug::LoginRequest req;
    req.set_name("zhang san");
    req.set_pwd("123456");

    //对象数据序列化 => char*
    std::string send_str;
    //将req序列化
    if(req.SerializeToString(&send_str)){
        std::cout << send_str.c_str() << std::endl;
    }

    //从send_str反序列化一个login请求对象
    fixbug::LoginRequest reqB;
    if(reqB.ParseFromString(send_str)){
        std::cout << reqB.name() << " " << reqB.pwd() << std::endl;
    }

    return 0;
}


