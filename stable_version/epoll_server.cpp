 #include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <string>
#include <string.h>
#include"find.cpp"
#include"LRU-time.cpp"
#include<time.h>
using namespace std; 
#define MAXEVENTS 64
 
//函数:
//功能:创建和绑定一个TCP socket
//参数:端口
//返回值:创建的socket
static int create_and_bind (char *port)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int s, sfd;
 
  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
  hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
  hints.ai_flags = AI_PASSIVE;     /* All interfaces */
 
  s = getaddrinfo (NULL, port, &hints, &result);
  if (s != 0)
    {
      fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
      return -1;
    }
  for (rp = result; rp != NULL; rp = rp->ai_next)
    {
      sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (sfd == -1)
        continue;
      s = bind (sfd, rp->ai_addr, rp->ai_addrlen);
      if (s == 0)
        {
          /* We managed to bind successfully! */
          break;
        }
      close (sfd);
    }
  if (rp == NULL)
    {
      fprintf (stderr, "Could not bind\n");
      return -1;
    }
  freeaddrinfo (result);
  return sfd;
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
    printf("发送成功：%s\n",server_num);

    
    return 1;
}
 
//函数
//功能:设置socket为非阻塞的
static int make_socket_non_blocking (int sfd)
{
  int flags, s;
  //得到文件状态标志
  flags = fcntl (sfd, F_GETFL, 0);
  if (flags == -1)
    {
      perror ("fcntl");
      return -1;
    }
  //设置文件状态标志
  flags |= O_NONBLOCK;
  s = fcntl (sfd, F_SETFL, flags);
  if (s == -1)
    {
      perror ("fcntl");
      return -1;
    }
 
  return 0;
}
 
//端口由参数argv[1]指定
int main (int argc, char *argv[])
{
  int sfd, s;
  int efd;
  struct epoll_event event;
  struct epoll_event *events;
  long int time_now = time(NULL);
  Find find;
  LRUCache lru(100);
  string ip = "81.70.55.99";

 
  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s [port]\n", argv[0]);
      exit (EXIT_FAILURE);
    }
  sfd = create_and_bind (argv[1]);
  if (sfd == -1) abort ();
  s = make_socket_non_blocking (sfd);
  if (s == -1) abort ();
  s = listen (sfd, SOMAXCONN);
  if (s == -1)
    {
      perror ("listen");
      abort ();
    }
  //除了参数size被忽略外,此函数和epoll_create完全相同
  efd = epoll_create1 (0);
  if (efd == -1)
    {
      perror ("epoll_create");
      abort ();
    }
  event.data.fd = sfd;
  event.events = EPOLLIN | EPOLLET;//读入,边缘触发方式
  s = epoll_ctl (efd, EPOLL_CTL_ADD, sfd, &event);
  if (s == -1)
    {
      perror ("epoll_ctl");
      abort ();
    }
  /* Buffer where events are returned */
  events =(struct epoll_event *) calloc (MAXEVENTS, sizeof event);
  /* The event loop */
  int h_n = 0;
  while (1)
    {
      int n, i;
      
      if(time(NULL)-time_now>5){ //每五秒发送心跳
        if(((string)argv[1]).substr(0,4)=="9999"&&h_n>5){
          send_message("offline:"+(string)argv[1],"6677");
          time_now+=1000;

        }else{

        time_now = time(NULL);
        string llru = boost::lexical_cast<string>(lru.size); 
        send_message("heart:"+(string)argv[1]+" 存储数量:"+llru,"6677");
        h_n +=1;
        printf("当前lru数量:%d\n",lru.size);
        }

      }

      n = epoll_wait (efd, events, MAXEVENTS, 500);
      for (i = 0; i < n; i++)
        {
          if ((events[i].events & EPOLLERR) ||
              (events[i].events & EPOLLHUP) ||
              (!(events[i].events & EPOLLIN)))
            {
              /* An error has occured on this fd, or the socket is not
                 ready for reading (why were we notified then?) */
              fprintf (stderr, "epoll error\n");
              close (events[i].data.fd);
              continue;
            }
          else if (sfd == events[i].data.fd)
            {
              /* We have a notification on the listening socket, which
                 means one or more incoming connections. */
              while (1)
                {
                  struct sockaddr in_addr;
                  socklen_t in_len;
                  int infd;
                  char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
                  in_len = sizeof in_addr;
                  infd = accept (sfd, &in_addr, &in_len);
                  if (infd == -1)
                    {
                      if ((errno == EAGAIN) ||
                          (errno == EWOULDBLOCK))
                        {
                          /* We have processed all incoming
                             connections. */
                          break;
                        }
                      else
                        {
                          perror ("accept");
                          break;
                        }
                    }
                                  //将地址转化为主机名或者服务名
                  s = getnameinfo (&in_addr, in_len,
                                   hbuf, sizeof hbuf,
                                   sbuf, sizeof sbuf,
                                   NI_NUMERICHOST | NI_NUMERICSERV);//flag参数:以数字名返回
                                  //主机地址和服务地址
                  if (s == 0)
                    {
                      printf("Accepted connection on descriptor %d "
                             "(host=%s, port=%s)\n", infd, hbuf, sbuf);
                    }
                  /* Make the incoming socket non-blocking and add it to the
                     list of fds to monitor. */
                  s = make_socket_non_blocking (infd);
                  if (s == -1)
                    abort ();
                  event.data.fd = infd;
                  event.events = EPOLLIN | EPOLLET;
                  s = epoll_ctl (efd, EPOLL_CTL_ADD, infd, &event);
                  if (s == -1)
                    {
                      perror ("epoll_ctl");
                      abort ();
                    }
                }
              continue;
            }
          else
            {
              /* We have data on the fd waiting to be read. Read and
                 display it. We must read whatever data is available
                 completely, as we are running in edge-triggered mode
                 and won't get a notification again for the same
                 data. */
              int done = 0;
              while (1)
                {
                  ssize_t count;
                  char buf[1024];
                  count = read (events[i].data.fd, buf, sizeof(buf));
                  if (count == -1)
                    {
                      /* If errno == EAGAIN, that means we have read all
                         data. So go back to the main loop. */
                      if (errno != EAGAIN)
                        {
                          perror ("read");
                          done = 1;
                        }
                      break;
                    }
                  else if (count == 0)
                    {
                      /* End of file. The remote has closed the
                         connection. */
                      done = 1;
                      break;
                    }
                  /* Write the buffer to standard output */
                  string out = buf;
                  out = out.substr(0,count);
                  printf("收到消息：%s\n",out.c_str());
                  if(out.substr(0,5)=="dist:"){//获得新分布
                    printf("收到新分布\n");
                    find.renew(out.substr(5));
                    printf("lru长度:%d\n",lru.size);
                    //新开线程负载数据迁移。
                    if(lru.size>0){ 
                      printf("启动数据迁移线程\n");
                      thread second(&LRUCache::sendtoserver,ref(lru),ref(find),(string)argv[1]) ;
                      second.detach();
                    }
                  }else if(out.substr(0,4)=="get:"){//来自client的get
                    string send_back;
                    if(find.findserver(out.substr(4))==(string)argv[1]){
                      send_back ="Value:" +lru.get(out.substr(4));
                    }else{
                      send_back = "denied";
                    }
                    if( send(events[i].data.fd, send_back.c_str(), strlen(send_back.c_str()), 0) < 0)
                    {
                      printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
                      return -4;
                    }
                  }else if(out.substr(0,5)=="send:"){//来自client的send
                    vector<string> key_value;
                    string send_back;
                    boost::split(key_value,out.substr(5),boost::is_any_of(" "));
                    //上锁
                    if(find.findserver(key_value[0])==(string)argv[1]){
                      send_back = "ok";
                      lru.put(key_value[0],key_value[1]);
                    }else{
                      send_back = "denied";
                    }
                    if( send(events[i].data.fd, send_back.c_str(), strlen(send_back.c_str()), 0) < 0)
                    {
                      printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
                      return -4;
                    }
                  }else if(out.substr(0,7)=="s_send:"){//来自server的send key value time
                    //上锁
                    vector<string> key_value_time;
                    boost::split(key_value_time,out.substr(7),boost::is_any_of(" "));
                    lru.s_send(key_value_time[0],key_value_time[1],atoi(key_value_time[2].c_str()));
                  }else {printf("消息格式错误\n");}
                }
              if (done)
                {
                  printf ("Closed connection on descriptor %d\n",
                          events[i].data.fd);
 
                  /* Closing the descriptor will make epoll remove it
                     from the set of descriptors which are monitored. */
                  close (events[i].data.fd);
                }
            }
        }
    }
  free (events);
  close (sfd);
  return EXIT_SUCCESS;
}