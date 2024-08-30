#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage: ./client ip port\n");
        return -1;
    }

    int sockfd;
    struct sockaddr_in servaddr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket() failed.");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));

    // Convert the IP string to binary form
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
    {
        perror("inet_pton() failed.");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
    {
        perror("connect() failed.");
        close(sockfd);
        return -1;
    }

    printf("connect ok.\n");

    // Send the first message
    send(sockfd, "send", sizeof("send"), 0);


    char buf[1024];
    while (true)
    {
        memset(buf,0,sizeof(buf));
        printf("please input:"); scanf("%s",buf);

        if (send(sockfd,buf,strlen(buf),0) <=0)
        { 
            printf("write() failed.\n");  close(sockfd);  return -1;
        }
    }
    close(sockfd);
    return 0;
}