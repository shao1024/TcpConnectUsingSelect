#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>

// 初始化服务端的IP和监听端口。 服务器在recv关闭时还需要关闭对应的套接字
int initserver(const char *IP,int port);
int recvsock = -1,sendsock = -1;
int main(int argc,char *argv[])
{
    if (argc != 3) { printf("usage: ./tcpselect ip port\n"); return -1; }

    // 初始化服务端用于监听的socket。
    int listensock = initserver(argv[1],atoi(argv[2]));
    printf("listensock=%d\n",listensock);

    if (listensock < 0) { printf("initserver() failed.\n"); return -1; }

    // 读事件：1）已连接队列中有已经准备好的socket（有新的客户端连上来了）；
    //               2）接收缓存中有数据可以读（对端发送的报文已到达）；
    //               3）tcp连接已断开（对端调用close()函数关闭了连接）。
    // 写事件：发送缓冲区没有满，可以写入数据（可以向对端发送报文）。

    fd_set readfds;                         // 需要监视读事件的socket的集合，大小为16字节（1024位）的bitmap。
    FD_ZERO(&readfds);                // 初始化readfds，把bitmap的每一位都置为0。
    FD_SET(listensock,&readfds);  // 把服务端用于监听的socket加入readfds。

    int maxfd=listensock;              // readfds中socket的最大值。

    while (true)        // 事件循环。
    {
  

        fd_set tmpfds=readfds;      // 在select()函数中，会修改bitmap，所以，要把readfds复制一份给tmpfds，再把tmpfds传给select()。

        // 调用select() 等待事件的发生（监视哪些socket发生了事件)。
        int infds=select(maxfd+1,&tmpfds,NULL,NULL,0); 

        // 如果infds<0，表示调用select()失败。
        if (infds<0)
        {
            perror("select() failed"); break;
        }

        // 如果infds>0，表示有事件发生，infds存放了已发生事件的个数。
        for (int eventfd=0;eventfd<=maxfd;eventfd++)
        {
            if (FD_ISSET(eventfd,&tmpfds)==0) continue;   // 如果eventfd在bitmap中的标志为0，表示它没有事件，continue

            // 如果发生事件的是listensock，表示已连接队列中有已经准备好的socket（有新的客户端连上来了）。
            if (eventfd==listensock)
            {
                struct sockaddr_in client;
                socklen_t len = sizeof(client);
                int clientsock = accept(listensock,(struct sockaddr*)&client,&len);
                if (clientsock < 0) { perror("accept() failed"); continue; }

                char buffer[1024];
                memset(buffer,0,sizeof(buffer));
                recv(clientsock,buffer,sizeof(buffer),0);
                printf ("accept client(socket=%d) ok. function = %s, ip = %s , port = %d\n",
                                    clientsock,buffer,inet_ntoa(client.sin_addr),ntohs(client.sin_port));
                if(strcmp(buffer,"send") == 0)
                {
                    sendsock = clientsock;
                }
                else if(strcmp(buffer,"receive") == 0)
                {
                    // 接受端的套接字不会发送消息，所以不进入select的位图
                    recvsock = clientsock;
                    continue;
                }

                FD_SET(clientsock,&readfds);                      // 把bitmap中新连上来的客户端的标志位置为1。

                if (maxfd<clientsock) maxfd=clientsock;    // 更新maxfd的值。
            }
            else
            {
                // 如果是客户端连接的sock有事件，表示接收缓存中有数据可以读（对端发送的报文已到达），或者有客户端已断开连接。
                char buffer[1024];                      // 存放从接收缓冲区中读取的数据。
                memset(buffer,0,sizeof(buffer));
                if (recv(eventfd,buffer,sizeof(buffer),0)<=0)
                {
                    // 如果客户端的连接已断开。
                    printf("client(eventfd=%d) disconnected.\n",eventfd);

                    close(eventfd);                         // 关闭客户端的socket

                    FD_CLR(eventfd,&readfds);     // 把bitmap中已关闭客户端的标志位清空。
          
                    if (eventfd == maxfd)              // 重新计算maxfd的值，注意，只有当eventfd==maxfd时才需要计算。
                    {
                        for (int ii=maxfd;ii>0;ii--)    // 从后面往前找。
                        {
                            if (FD_ISSET(ii,&readfds))
                            {
                                maxfd = ii; break;
                            }
                        }
                    }
                }
                else
                {
                    // 如果客户端有报文发过来。
                    printf("send(eventfd=%d):%s\n",eventfd,buffer);
                    // 此时是接受端还没连上来，发送端已经在发消息了
                    if(recvsock == -1)
                    {
                        printf("error! recv socket not ready\n");
                        continue;
                    } 
                    // 把接收到的报文内容发给接收端
                    send(recvsock,buffer,strlen(buffer),0);
                }
            }
        }
    }

    return 0;
}

// 初始化服务端的监听端口。
int initserver(const char *IP,int port)
{
    int sock = socket(AF_INET,SOCK_STREAM,0);
    if (sock < 0)
    {
        perror("socket() failed"); return -1;
    }

    int opt = 1; unsigned int len = sizeof(opt);
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,len);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, IP, &servaddr.sin_addr);
    //servaddr.sin_addr.s_addr = inet_addr(IP);
    servaddr.sin_port = htons(port);

    if (bind(sock,(struct sockaddr *)&servaddr,sizeof(servaddr)) < 0 )
    {
        perror("bind() failed"); close(sock); return -1;
    }

    if (listen(sock,5) == -1 )
    {
        perror("listen() failed"); close(sock); return -1;
    }

    return sock;
}
