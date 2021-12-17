#include<vector>
#include<string.h>
#include<string>
#include<boost/algorithm/string.hpp>
#include<boost/lexical_cast.hpp>
#include<sys/socket.h>
#include<errno.h>
// #include "find.cpp"

#include<netinet/in.h>
#include <unistd.h>
#include<arpa/inet.h>
#include<map>
using namespace std;


class hashslot{
    public:
    vector<int> slot;
    map<int,string> itos;
    map<string,int> stoi;
    hashslot(int);
    void add_server(string);
    void del_server(string);
    void lost_server(string);
    int  send_server();
    string output();
    string del_server_ip;

    
    private:
    void add_map(string);
    void del_map(string);
};

hashslot::hashslot(int cap){
    slot =  vector<int> (cap,-1); 
}
void hashslot::add_map(string ip){
    int num = itos.size();
    itos[num+1]=ip;
    printf("添加server:%s\n",ip.c_str());
    stoi[ip] = num+1;
}
void hashslot::del_map(string ip){
    int num = stoi[ip];
    stoi.erase(ip);
    itos.erase(num);
}
void hashslot::add_server(string ip){
    if(stoi.size()==0){  //第一个全部填充 IP代号
        add_map(ip);
        slot =  vector<int> (slot.size(),stoi[ip]);
    }else{  //第N个把前N-1个 抽cap/(n*(n-1))变为 IP代号
        // add_map(ip);
        map<int,int> change_num;
        map<int,string>::iterator i;
        int n = itos.size();
        for (i= itos.begin();i!=itos.end();i++){
            change_num[i->first] = slot.size()/(n*(n+1));
        }
        add_map(ip);
        for(int j = 0;j<slot.size();j++){
            if(change_num[slot[j]]>0){
                change_num[slot[j]]--;
                slot[j]= stoi[ip];
            }
        }
    }

}
void hashslot::del_server(string ip){
    if(stoi.count(ip)<=0) return;
    if(stoi.size()==1){  //第一个全部填充 IP代号
        del_map(ip);
        slot =  vector<int> (slot.size(),0);
    }else{  //第N个把前N-1个 抽cap/(n*(n-1))变为 IP代号
        // add_map(ip);
        map<int,string>::iterator i;
        int n = itos.size();
        int num = stoi[ip];
        vector<int> replace;
        for (i= itos.begin();i!=itos.end();i++){
            if(i->first!=num){
            for(int k=0;k<slot.size()/(n*(n-1));k++){
                replace.push_back(i->first);
            }
            }
        }
        del_map(ip);
        int ii = 0;
        for(int j = 0;j<slot.size();j++){
            if(slot[j]==num ){
                slot[j]=replace[ii];
                ii++;
                
            }
        }
        del_server_ip = ip;
    }


}
void hashslot::lost_server(string ip){
    del_server(ip);
    del_server_ip = "";

    
}
string hashslot::output(){
    string outa="";
    string outb="";
    for (int m = 0 ;m<slot.size();m++){
        outa+=boost::lexical_cast<string>(slot[m]);
    }
    map<int,string>::iterator i;
    for ( i = itos.begin() ;i!=itos.end();i++){
        outb+=boost::lexical_cast<string>(i->first)+":"+i->second+" ";
    }
    outb=outb.substr(0,outb.size()-1);
    string out =outa+"+"+outb;
    printf("%s\n",out.c_str());
    return out;
}

int send_message(string dist_s,const char* port){
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
    printf("发送分布成功：%s\n",port);

    
    return 1;
}



int hashslot::send_server(){ ///向offline外的已知所有server发送最新的分布
    string dist_s = "dist:"+this->output() ;
    map<int,string>::iterator i;
    int out;
    for ( i = itos.begin() ;i!=itos.end();i++){
        out =send_message(dist_s,(i->second).c_str());
    }
    if (del_server_ip.size()>0){
        out = send_message(dist_s,del_server_ip.c_str());
        del_server_ip="";
    }
    return out;
}



// int main(){
//     Find find;
//     hashslot hash(120); //初始化容量；
//     hash.output();
//     hash.add_server("a"); //上线server a 
//     hash.output();
//     hash.add_server("b"); //上线server b 
//     hash.output();
//     hash.add_server("c"); //上线server c 
//     hash.output();
//     hash.add_server("d"); //上线server d 
//     find.renew(hash.output().substr(13));
//     printf("%s\n",find.findserver("aaaaaaaaa").c_str());
//     hash.del_server("c"); //主动下线server c 
//     hash.output();
//     hash.lost_server("b"); //掉线server b 
//     hash.output();


// }