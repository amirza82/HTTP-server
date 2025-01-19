#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "head.h"

int main(int argc, char **argv)
{
    int sockfd, n;
    int sendbytes;
    int port;
    struct sockaddr_in servaddr;
    char sendline[MAXLINE];
    char recvline[MAXLINE];

    port = 8080;
    if (argc == 3)
        port = atoi(argv[2]);
    else if (argc != 2)
        DIE("usage: client {server address} [port]");

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        DIE("faild to create socket");

    memset(&servaddr, 0, sizeof (servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) 
        DIE("inet_pton can't parse server address");

    if (connect(sockfd, (SA *) &servaddr, sizeof (servaddr)) < 0)
        DIE("connection faild!");

    sprintf(sendline, "GET / HTTP/1.0\r\n\r\n");
    sendbytes = strlen(sendline);

    if (write(sockfd, sendline, sendbytes) != sendbytes)
        DIE("write error!");

    memset(recvline, 0, sizeof (recvline));

    //Read server response
    while ((n = read(sockfd, recvline, sizeof (recvline))) > 0) {
        printf("%s", recvline);
        memset(recvline, 0, sizeof (recvline));
    }

    if (n < 0)
        DIE("read error!");

    return 0;
}
