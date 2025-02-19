#include "json.hpp"
using json = nlohmann::json;

#include<iostream>
#include<vector>
#include<map>
#include<string>
using namespace std;

/**
 * Json数据序列化，把我们想要打包的数据或者对象，直接处理成Json字符串
 */
string func1()
{
    json js;
    //无序哈希表
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello what are you doing now?";
    cout << js << endl;
    string sendBuf = js.dump();
    cout << sendBuf.c_str() << endl;
    return sendBuf;
}

string func2()
{
    json js;
    //添加数组
    js["id"] = {1,2,3,4,5};
    //添加 key-value
    js["name"] = "zhang san";
    //添加对象
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["liu shuo"] = "hello china";
    //上面等同于下面这句一次性添加数组对象
    js["msg"] = {{"zhang san","hello world"},{"liu shuo","hello china"}};
    cout << js << endl;
    return js.dump();
}

//json序列化代码3
void func3()
{
    json js;
    //直接序列化一个vector容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);
    js["list"] = vec;

    map<int,string> m;
    m.insert({1,"黄山"});
    m.insert({2,"华山"});
    m.insert({3,"泰山"});
    js["path"] = m;
    cout << js <<endl;

    string sendBuf = js.dump();
    cout << sendBuf <<endl;
}

int main()
{
    string recvBuf1 = func1();
    //数据的反序列化    json字符串 =》 反序列化  数据对象（看作容器，方便访问）
    json jsbuf1 = json::parse(recvBuf1);
    cout << jsbuf1["msg_type"] << endl;
    cout << jsbuf1["from"] << endl;
    cout << jsbuf1["to"] << endl;
    cout << jsbuf1["msg"] << endl;

    string recvBuf2 = func2();
    //数据的反序列化    json字符串 =》 反序列化  数据对象（看作容器，方便访问）
    json jsbuf2 = json::parse(recvBuf2);
    cout << jsbuf2["id"] << endl;
    cout << jsbuf2["id"][2] << endl;
    cout << jsbuf2["name"] << endl;
    cout << jsbuf2["msg"] << endl;
    return 0;
}

