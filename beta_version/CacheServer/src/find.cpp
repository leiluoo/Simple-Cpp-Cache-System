/*
 * @Author: your name
 * @Date: 2021-12-12 13:06:11
 * @LastEditTime: 2021-12-17 13:32:42
 * @LastEditors: ArthurOyll
 * @Description: 
 * @FilePath: /TencentTopic/version2/CacheServer/src/find.cpp
 */
#include "find.hpp"

//findserver方法：输入index 输出节点的地址
std::string Find::findserver(std::string key){
    if(dist.size()!=120) {
        printf("分布数据有误:%ld\n",dist.size());
        exit(0);
        }
    std::string sha2= picosha2::hash256_hex_string(key);
    int index = stoi(sha2.substr(59),0,16)%120;
    char ip_s = dist[index];
    int ip_num = atoi(&ip_s);
    for (int i = 0;i<ip.size();i++){
        int n = atoi(ip[i].substr(0,1).c_str());
        if(n == ip_num ){
            return(ip[i].substr(2));
        }
    }
    return "-1";
}
//renew方法：从ab分布中更新Find类
void Find::renew(std::string distribution){
    printf("find更新\n");
    std::vector<std::string> vex;
    boost::split(vex,distribution,boost::is_any_of("+"));
    dist = vex[0] ;
    printf("dist长度：%ld\n",dist.size());
    if(vex[1].size()==0){
        
    printf("find更新后无server\n");
    }
    boost::split(ip,vex[1],boost::is_any_of(" "));
    printf("find更新后共有%ld个server\n",ip.size());
}