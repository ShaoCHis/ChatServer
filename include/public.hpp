#ifndef PUBLIC_H
#define PUBLIC_H

/**
 * server和client的公共文件
 */

//消息类型
/**
 * enum 与 enum class 区别
 * enum具有隐式int转换，而enum class 不具备，如果需要使用数值，需要进行强制类型转换
 * enum class 可以防止命名空间污染
 * static_cast<type>(experssion)
 * dynamic_cast<type>(experssion)
 * reinterpret_cast<type>(experssion)
 * const_cast<type>(experssion)
 */
enum EnMsgType
{
    LOGIN_MSG = 1,  //登陆消息
    LOGIN_MSG_ACK,  //登陆成功
    REG_MSG,        //注册消息
    REG_MSG_ACK,    //注册成功
    ONE_CHAT_MSG,   //聊天消息
    ADD_FRIEND_MSG, //添加好友
    
};


#endif