#include "csapp.h"

void echo(int connfd) {
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    //rio와 connfd 연결
    Rio_readinitb(&rio, connfd);
    //connfd의 변경을 기다림(네트워크 대기)
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){
        printf("%d byte를 받았습니다\n", (int)n);
        //connfd 변경을 통해 수신.
        Rio_writen(connfd, buf, n);
    }
}