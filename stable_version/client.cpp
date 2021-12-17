#include<iostream>
#include<ctime>
#include<boost/lexical_cast.hpp>
#include<boost/algorithm/string.hpp>
#include<vector>
#include "picosha2.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include <unistd.h>
#include<arpa/inet.h>
#include<map>
#include "find.cpp"
using namespace std;
//从key分布中获取当前index所在节点的类

//Client向Cache_server获取Value,输入参数（返回结果，要获取的key，节点地址）
//返回int：1：成功获取 -1：地址格式错误 -2：套接字创建失败 -3：连接Server端口失败 -4：发送key失败 -5：接收Value失败 -6：返回格式错误（Value:）
int get_message(string &out,string key,const char* port){
    int    sockfd, n;
    char    recvline[4096], sendline[4096];
    struct sockaddr_in    servaddr;
    string message = "get:"+key;
    const char* server_num = "127.0.0.1";


    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(port));
    if( inet_pton(AF_INET, server_num, &servaddr.sin_addr) <= 0){
    printf("inet_pton error for %s\n",server_num);
    return -1;
    }
    
    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
    return -2;
    }
    /* code */
    if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
    printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
    return -3;
    }

    printf("send msg to server: %s\n",message.data());
    if( send(sockfd, message.c_str(), strlen(message.c_str()), 0) < 0)
    {
    printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
    return -4;
    }
    int count =  recv(sockfd,recvline,4096,0);
    if(count<0){
        printf("recv msg error: %s(errno: %d)\n", strerror(errno), errno);
        return -5;
    }
    close(sockfd);
    string recv_s = recvline;
    recv_s = recv_s.substr(0,count);
    printf("返回：%s\n",recv_s.c_str());    
    // value= value.substr(0,)
    if(recv_s.substr(0,6)=="Value:"){
    out = recv_s.substr(6);
    return 1;
    }else if(recv_s.substr(0,6) =="denied"){
        printf("服务器地址错误\n");
        return -7;
    }else{
        printf("返回Value格式错误！\n");
        return -6;
    }
}

//Client向Cache_server发送Key-Value,输入参数（key，value,节点地址）
//返回int：1：成功获取 -1：地址格式错误 -2：套接字创建失败 -3：连接Server端口失败 -4：发送key-value失败 -5：接收回应失败 -6：Server返回发送失败错误
int send_message(string key,string value,const char* port){
    int    sockfd, n;
    char    recvline[4096], sendline[4096];
    struct sockaddr_in    servaddr;
    string message ="send:" +key+" "+value;
    const char* server_num = "127.0.0.1";


    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(port));
    if( inet_pton(AF_INET, server_num, &servaddr.sin_addr) <= 0){
    printf("inet_pton error for %s\n",server_num);
    return -1;
    }
    
    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
    return -2;
    }
    /* code */
    if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
    printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
    return -3;
    }

    printf("send msg to server: %s\n",message.data());
    if( send(sockfd, message.c_str(), strlen(message.c_str()), 0) < 0)
    {
    printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
    return -4;
    }
    int count =  recv(sockfd,recvline,4096,0);
    if(count<0){
        printf("recv msg error: %s(errno: %d)\n", strerror(errno), errno);
        return -5;
    }
    close(sockfd);
    string recv_s = recvline;
    recv_s = recv_s.substr(0,count);
    printf("返回：%s\n",recv_s.c_str());    
    if(recv_s.substr(0,6) =="denied"){
        printf("服务器地址错误\n");
        return -7;
    }else if(recv_s.substr(0,2) =="ok"){
        printf("发送成功\n");
        return 1;
    }else {
        printf("返回消息格式错误\n");
        return -6;
    }
}

//Client主动向master 请求最新分布，输入参数（返回a,b分布)
//返回int：1：成功获取 -1：地址格式错误 -2：套接字创建失败 -3：连接Server端口失败 -4：发送key-value失败 -5：接收回应失败 -6：Server返回发送失败错误
int get_distribution(Find &findserver){
    string ip = "127.0.0.1"; //MasterIP
    int    sockfd, n;
    char    recvline[4096],recvline1[4096], sendline[4096];
    struct sockaddr_in    servaddr;
    string message = "get_distribution";


    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(6677);
    if( inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr) <= 0){
    printf("inet_pton error for %s\n",ip.c_str());
    return -1;
    }
    
    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
    return -2;
    }
    /* code */
    if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
    printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
    return -3;
    }

    printf("send msg to server: %s\n",message.data());
    if( send(sockfd, message.c_str(), strlen(message.c_str()), 0) < 0)
    {
    printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
    return -4;
    }
    if( recv(sockfd,recvline,4096,0)<0){
        printf("recv msg error: %s(errno: %d)\n", strerror(errno), errno);
        return -5;
    }
    
    string a = recvline; //转为字符串
    printf("收到分布:%s\n",a.c_str());
    if(a.substr(0,13)!="distribution:"){
        printf("key分布格式错误\n");
        return -1;
    }
    findserver.renew(a.substr(13)) ; // a分布,b分布"distribution:"
    printf("key分布接收成功\n");
    close(sockfd);
    return 1;
}



int main(){
    srand(time(0));
    int initial_num = 110;
    int main_loop = 1000;

    map<string,int> lose_con;
    string value_got;
    Find findserver; //开启分布算法
    get_distribution(findserver); //从master获取key分布
    //初始化：向client写入数据指定key和随机value
    for (int i=0;i<initial_num;i++){  
        usleep(500000);
        int value = rand();
        string key_s = boost::lexical_cast<string>(i);
        string value_s = boost::lexical_cast<string>(value);
        string addr = findserver.findserver(key_s);
        int info = send_message(key_s,value_s,addr.c_str());
        if ( info==1){
            printf("初始化写入成功:%d 发往:%s\n",i,addr.c_str());
        }else if(info==-7||info==-3){
            if(get_distribution(findserver)!=1){
                printf("分布获取失败\n");
                exit(1);
            }
        }
       
    }
    printf("初始化完毕");
    for(int i=0;i<main_loop;i++){
        usleep(500000);
        //读取随机key
        int key = rand()%initial_num;
        string key_s = boost::lexical_cast<string>(key);
        string addr = findserver.findserver(key_s);
        int info = get_message(value_got,key_s,addr.c_str());
        if(info==1){
            printf("获得key:%d value:%s 来自%s\n",key,value_got.c_str(),addr.c_str());
        }else if(info==-7||info==-3){
            if(get_distribution(findserver)!=1){
                printf("分布获取失败\n");
                exit(1);
            }
        
        }
        //写入随机key-value
        key = rand()%initial_num;
        int value = rand();
        key_s = boost::lexical_cast<string>(key);
        string value_s = boost::lexical_cast<string>(value);
         info = send_message(key_s,value_s,findserver.findserver(key_s).c_str());
        if (info==1){
            printf("写入key:%d value:%d 发往%s\n",key,value,addr.c_str());
        }else if(info==-7||info==-3){
            if(get_distribution(findserver)!=1){
                printf("分布获取失败\n");
                exit(1);
            }
        }

    }

    // Find findserver("1-4-3-12-2-8-6-24-1-4-3-12-2-8-6-24","1-2-1-3-1-2-1-4-1-2-1-3-1-2-1-5");
    
    
}