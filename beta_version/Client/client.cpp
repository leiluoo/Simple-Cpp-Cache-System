#include <iostream>
#include <ctime>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <vector>
#include "picosha2.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <map>
#include "find.hpp"
#include "TcpSocket.hpp"
using namespace std;
//从key分布中获取当前index所在节点的类

//Client向Cache_server获取Value,输入参数（返回结果，要获取的key，节点地址）
//返回int：1：成功获取 -1：地址格式错误 -2：套接字创建失败 -3：连接Server端口失败 -4：发送key失败 -5：接收Value失败 -6：返回格式错误（Value:）
int get_message(string &out, string key, const char *port)
{
    string message = key;
    TcpSocket tcp;
    tcp.connectToHost("127.0.0.1", atoi(port));
    tcp.sendMsg(key);
    string recv_s = tcp.recvMsg();
    cout << "返回: " << recv_s << endl;
    // value= value.substr(0,)
    if (recv_s.substr(0, 6) == "Value:")
    {
        //out = recv_s.substr(6);
        return 1;
    }
    else if (recv_s == "denied")
    {
        printf("服务器地址错误\n");
        return -7;
    }
    else if (recv_s == "Not Found")
    {
        cout << "value不存在" << endl;
    }
    else
    {
        printf("返回Value格式错误！\n");
        return -6;
    }
}

//Client向Cache_server发送Key-Value,输入参数（key，value,节点地址）
//返回int：1：成功获取 -1：地址格式错误 -2：套接字创建失败 -3：连接Server端口失败 -4：发送key-value失败 -5：接收回应失败 -6：Server返回发送失败错误
int send_message(string key, string value, const char *port)
{
    string message = key + " " + value;
    const char *server_num = "127.0.0.1";

    TcpSocket tcp;
    tcp.connectToHost("127.0.0.1", atoi(port));
    tcp.sendMsg(message);
    string recv_s = tcp.recvMsg();
    cout << "返回: " << recv_s;
    if (recv_s == "denied")
    {
        printf("服务器地址错误\n");
        return -7;
    }
    else if (recv_s == "ok")
    {
        printf("发送成功\n");
        return 1;
    }
    else
    {
        printf("返回消息格式错误\n");
        return -6;
    }
}

//Client主动向master 请求最新分布，输入参数（返回a,b分布)
//返回int：1：成功获取 -1：地址格式错误 -2：套接字创建失败 -3：连接Server端口失败 -4：发送key-value失败 -5：接收回应失败 -6：Server返回发送失败错误
int get_distribution(Find &findserver)
{
    string ip = "127.0.0.1"; //MasterIP
    string message = "get_distribution";
    TcpSocket tcp;
    tcp.connectToHost(ip, 6677);
    tcp.sendMsg(message);
    string a = tcp.recvMsg(); 
    printf("收到分布:%s\n", a.c_str());
    if (a.substr(0, 13) != "distribution:")
    {
        printf("key分布格式错误\n");
        return -1;
    }
    findserver.renew(a.substr(13)); // a分布,b分布"distribution:"
    printf("key分布接收成功\n");
    return 1;
}

int main()
{
    srand(time(0));
    int initial_num = 110;
    int main_loop = 1000;

    map<string, int> lose_con;
    string value_got;
    Find findserver;              //开启分布算法
    get_distribution(findserver); //从master获取key分布
    //初始化：向client写入数据指定key和随机value
    for (int i = 0; i < initial_num; i++)
    {
        usleep(1000000);
        int value = rand();
        string key_s = boost::lexical_cast<string>(i);
        string value_s = boost::lexical_cast<string>(value);
        string port = findserver.findserver(key_s);
        *(--port.end()) -= 1;
        int info = send_message(key_s, value_s, port.c_str());
        if (info == 1)
        {
            printf("初始化写入成功:%d 发往:%s\n", i, port.c_str());
        }
        else if (info == -7 || info == -3)
        {
            if (get_distribution(findserver) != 1)
            {
                printf("分布获取失败\n");
                exit(1);
            }
        }
    }
    printf("初始化完毕");
    for (int i = 0; i < main_loop; i++)
    {
        usleep(500000);
        //读取随机key
        int key = rand() % initial_num;
        string key_s = boost::lexical_cast<string>(key);
        string addr = findserver.findserver(key_s);
        int info = get_message(value_got, key_s, addr.c_str());
        if (info == 1)
        {
            printf("获得key:%d value:%s 来自%s\n", key, value_got.c_str(), addr.c_str());
        }
        else if (info == -7 || info == -3)
        {
            if (get_distribution(findserver) != 1)
            {
                printf("分布获取失败\n");
                exit(1);
            }
        }
        //写入随机key-value
        key = rand() % initial_num;
        int value = rand();
        key_s = boost::lexical_cast<string>(key);
        string value_s = boost::lexical_cast<string>(value);
        string port = findserver.findserver(key_s).c_str();
        *(--port.end()) -= 1;
        info = send_message(key_s, value_s, port.c_str());
        if (info == 1)
        {
            printf("写入key:%d value:%d 发往%s\n", key, value, addr.c_str());
        }
        else if (info == -7 || info == -3)
        {
            if (get_distribution(findserver) != 1)
            {
                printf("分布获取失败\n");
                exit(1);
            }
        }
    }

    // Find findserver("1-4-3-12-2-8-6-24-1-4-3-12-2-8-6-24","1-2-1-3-1-2-1-4-1-2-1-3-1-2-1-5");
}