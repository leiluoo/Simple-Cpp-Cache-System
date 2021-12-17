#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <string>
#include <map>
#include "hashslot.hpp"
#include "TcpSocket.hpp"
using namespace std;

#define MAXEVENTS 64

//函数:
//功能:创建和绑定一个TCP socket
//参数:端口
//返回值:创建的socket
static int
create_and_bind(char *port)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int s, sfd;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;	 /* Return IPv4 and IPv6 choices */
	hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
	hints.ai_flags = AI_PASSIVE;	 /* All interfaces */

	s = getaddrinfo(NULL, port, &hints, &result);
	if (s != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
			continue;

		s = bind(sfd, rp->ai_addr, rp->ai_addrlen);
		if (s == 0)
		{
			/* We managed to bind successfully! */
			break;
		}

		close(sfd);
	}

	if (rp == NULL)
	{
		fprintf(stderr, "Could not bind\n");
		return -1;
	}

	freeaddrinfo(result);

	return sfd;
}

//函数
//功能:设置socket为非阻塞的
static int
make_socket_non_blocking(int sfd)
{
	int flags, s;

	//得到文件状态标志
	flags = fcntl(sfd, F_GETFL, 0);
	if (flags == -1)
	{
		perror("fcntl");
		return -1;
	}

	//设置文件状态标志
	flags |= O_NONBLOCK;
	s = fcntl(sfd, F_SETFL, flags);
	if (s == -1)
	{
		perror("fcntl");
		return -1;
	}

	return 0;
}

//端口由参数argv[1]指定
int main(int argc, char *argv[])
{
	int sfd, s;
	int efd;
	struct epoll_event event;
	struct epoll_event *events;
	const char *ok = "OK";
	map<string, long int> heart;
	// map<string,long int>::iterator h_iter;
	long int time_now = time(NULL);
	hashslot dist(120);

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	sfd = create_and_bind(argv[1]);
	if (sfd == -1)
		abort();

	s = make_socket_non_blocking(sfd);
	if (s == -1)
		abort();

	s = listen(sfd, SOMAXCONN);
	if (s == -1)
	{
		perror("listen");
		abort();
	}

	//除了参数size被忽略外,此函数和epoll_create完全相同
	efd = epoll_create1(0);
	if (efd == -1)
	{
		perror("epoll_create");
		abort();
	}

	event.data.fd = sfd;
	event.events = EPOLLIN | EPOLLET; //读入,边缘触发方式
	s = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);
	if (s == -1)
	{
		perror("epoll_ctl");
		abort();
	}

	/* Buffer where events are returned */
	events = (struct epoll_event *)calloc(MAXEVENTS, sizeof event);

	/* The event loop */
	printf("初始化完成\n");
	while (1)
	{
		int n, i;

		if (time(NULL) - time_now > 5)
		{ //每五秒检查心跳
			// printf("检查心跳\n");
			time_now = time(NULL);
			for (auto h_iter = heart.begin(), next_i = h_iter; h_iter != heart.end(); h_iter = next_i)
			{
				++next_i;
				if (time_now - h_iter->second > 15)
				{ // 十五秒未收到心跳则server掉线
					//h_iter->first IP掉线
					dist.lost_server(h_iter->first);
					printf("%s掉线\n", (h_iter->first).c_str());
					heart.erase(h_iter);
					// dist.output();
					dist.send_server();
				}
			}
		}

		n = epoll_wait(efd, events, MAXEVENTS, 1000);
		for (i = 0; i < n; i++)
		{
			if ((events[i].events & EPOLLERR) ||
				(events[i].events & EPOLLHUP) ||
				(!(events[i].events & EPOLLIN)))
			{
				/* An error has occured on this fd, or the socket is not
                 ready for reading (why were we notified then?) */
				fprintf(stderr, "epoll error\n");
				close(events[i].data.fd);
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
					infd = accept(sfd, &in_addr, &in_len);
					if (infd == -1)
					{
						if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
						{
							/* We have processed all incoming
                             connections. */
							break;
						}
						else
						{
							perror("accept");
							break;
						}
					}
					//将地址转化为主机名或者服务名
					s = getnameinfo(&in_addr, in_len,
									hbuf, sizeof hbuf,
									sbuf, sizeof sbuf,
									NI_NUMERICHOST | NI_NUMERICSERV); //flag参数:以数字名返回
																	  //主机地址和服务地址

					if (s == 0)
					{
						printf("Accepted connection on descriptor %d "
							   "(host=%s, port=%s)\n",
							   infd, hbuf, sbuf);
					}

					/* Make the incoming socket non-blocking and add it to the
                     list of fds to monitor. */
					s = make_socket_non_blocking(infd);
					if (s == -1)
						abort();

					event.data.fd = infd;
					event.events = EPOLLIN | EPOLLET;
					s = epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event);
					if (s == -1)
					{
						perror("epoll_ctl");
						abort();
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
					TcpSocket tcp(events[i].data.fd);
					string msg = tcp.recvMsg();

					cout << "收到消息: " << msg << endl;

					if (msg.substr(0, 16) == "get_distribution")
					{ //返回分布
						string dist_return = "distribution:" + dist.output();
						tcp.sendMsg(dist_return);
						close(events[i].data.fd);
						break;
					}
					else if (msg.substr(0, 6) == "heart:")
					{
						string ip = msg.substr(6, 4);
						if (heart[ip] == 0)
						{
							//上线IP
							dist.add_server(ip);
							dist.send_server();
						}

						heart[ip] = time(NULL);
						close(events[i].data.fd);
						break;
					}
					else if (msg.substr(0, 8) == "offline:")
					{
						//下线IP
						string ip = msg.substr(8);
						printf("下线IP%s\n", ip.c_str());
						dist.del_server(ip);
						dist.send_server();
						heart.erase(ip);
						close(events[i].data.fd);
						break;
					}
					else
					{
						cout << "消息格式错误" << endl;
						exit(0);
						close(events[i].data.fd);
						break;
					}
				}

				if (done)
				{
					printf("Closed connection on descriptor %d\n",
						   events[i].data.fd);

					/* Closing the descriptor will make epoll remove it
                     from the set of descriptors which are monitored. */
					close(events[i].data.fd);
				}
			}
		}
	}

	free(events);

	close(sfd);

	return EXIT_SUCCESS;
}