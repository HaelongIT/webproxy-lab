#include "csapp.h"
#include <stdio.h>

int main(int argc, char **argv) {
// argc : 인자의 갯수 , argv : 프로그램의 리스트
    // printf("client\n");

    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <호스트> <포트>\n", argv[0]);
        // argv[0] : 프로그램명
        // stderr에 출력
        exit(0);
    }
    host = argv[1];
    prot = argv[2];

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }
    Close(clientfd);
    exit(0);

    return 0;
}