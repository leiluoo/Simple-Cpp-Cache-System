#include<unordered_map>
#include<shared_mutex>
#include<mutex>
#include<thread>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string>
 #include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
// #include "find.cpp"
#include<boost/lexical_cast.hpp>

using namespace std;
int send_message1(string dist_s,const char* port){
    int    sockfd, n;
    char    recvline[4096], sendline[4096];
    struct sockaddr_in    servaddr;
    string message = dist_s;
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
    close(sockfd);
    printf("发送成功：%s\n",port);

    
    return 1;
}

struct time_value {
    long int time;
    string value;
    time_value(long int _time,string _value): time(_time),value(_value) {}
};

struct DLinkedNode {
    string key;
    time_value tv;
    DLinkedNode* prev;
    DLinkedNode* next;
    DLinkedNode(): key(""), tv(0,""), prev(nullptr), next(nullptr) {}
    DLinkedNode(string _key, time_value _value): key(_key), tv(_value), prev(nullptr), next(nullptr) {}
};



class LRUCache {
private:
    unordered_map<string, DLinkedNode*> cache;
    DLinkedNode* head;
    DLinkedNode* tail;
    int capacity;
    mutable shared_mutex mutex_;

public:
    int size;

    LRUCache(int _capacity): capacity(_capacity), size(0) {
        // 使用伪头部和伪尾部节点
        head = new DLinkedNode();
        tail = new DLinkedNode();
        head->next = tail;
        tail->prev = head;
    }
    void s_send(string key,string value,long int time){
        printf("收到数据迁移,lru当前长度为：%d\n",size);
        unique_lock<shared_mutex> lock(mutex_);
        if (cache.count(key)) {
            return ;
        }
        time_value t = {time,value};
        DLinkedNode* node = new DLinkedNode(key, t);
        if(size <=0){
            cache[key] = node;
            addToHead(node);
            ++size;
            return;
        }
        for(DLinkedNode* pointer = tail->prev;pointer!=head;pointer = pointer->prev){
            if((pointer->tv).time>=t.time){
                node->prev=pointer;
                node->next = pointer->next;
                pointer->next->prev =node;
                pointer->next = node;
                cache[key] = node;
                ++size;
                if (size > capacity) {
                    // 如果超出容量，删除双向链表的尾部节点
                    DLinkedNode* removed = removeTail();
                    // 删除哈希表中对应的项
                    cache.erase(removed->key);
                    // 防止内存泄漏
                    delete removed;
                    --size;
                }
                return;
            }
        }
        if((head->next->tv).time<t.time){
            cache[key] = node;
            // 添加至双向链表的头部
            addToHead(node);
            ++size;
            if (size > capacity) {
                // 如果超出容量，删除双向链表的尾部节点
                DLinkedNode* removed = removeTail();
                // 删除哈希表中对应的项
                cache.erase(removed->key);
                // 防止内存泄漏
                delete removed;
                --size;
            }
        }
        
    }
    
    string get(string key) {
        shared_lock<shared_mutex> lock(mutex_);
        printf("读取key:%s ",key.c_str());
        if (!cache.count(key)) {
            return "notfound";
        }
        // 如果 key 存在，先通过哈希表定位，再移到头部
        DLinkedNode* node = cache[key];
        (node->tv).time = time(NULL);
        moveToHead(node);
        string out = (node->tv).value;
        
        return out;
    }
    
    void put(string key, string value) {
        unique_lock<shared_mutex> lock(mutex_);
        printf("写入key:%s value:%s\n",key.c_str(),value.c_str());
        if (!cache.count(key)) {
            // 如果 key 不存在，创建一个新的节点
            DLinkedNode* node = new DLinkedNode(key, time_value(time(NULL),value));
            // 添加进哈希表
            cache[key] = node;
            // 添加至双向链表的头部
            addToHead(node);
            ++size;
            if (size > capacity) {
                // 如果超出容量，删除双向链表的尾部节点
                DLinkedNode* removed = removeTail();
                // 删除哈希表中对应的项
                cache.erase(removed->key);
                // 防止内存泄漏
                delete removed;
                --size;
            }
        }
        else {
            // 如果 key 存在，先通过哈希表定位，再修改 value，并移到头部
            DLinkedNode* node = cache[key];
            (node->tv).value = value;
            (node->tv).time = time(NULL);
            moveToHead(node);
        }
        
    }

    void sendtoserver(Find &ff,string ip){
        int out;
        if(size<=0) return  ;
        for(DLinkedNode* pointer = head->next,*nn=pointer;pointer!=tail;pointer = nn){
            unique_lock<shared_mutex> lock(mutex_);
            nn=pointer->next;
            string ip_=ff.findserver(pointer->key);
            if(ip_!=ip){
            printf("数据迁移发送key:%s\n",(pointer->key).c_str());
                string dist = "s_send:"+pointer->key+" "+(pointer->tv).value+" "+boost::lexical_cast<string>((pointer->tv).time);
                out = send_message1(dist,ip_.c_str());
                //需要添加删除，避免越界
                removeNode(pointer);
                cache.erase(pointer->key);

                
                delete pointer;
                --size;
                //删除元素
            }
            lock.unlock();
            
            }
        
        
        return  ;

    }
    

    void addToHead(DLinkedNode* node) {
        node->prev = head;
        node->next = head->next;
        head->next->prev = node;
        head->next = node;
    }
    
    void removeNode(DLinkedNode* node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void moveToHead(DLinkedNode* node) {
        removeNode(node);
        addToHead(node);
    }

    DLinkedNode* removeTail() {
        DLinkedNode* node = tail->prev;
        removeNode(node);
        return node;
    }
};
