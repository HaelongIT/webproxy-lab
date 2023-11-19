#include "csapp.h"
#include <stdio.h>


// 개인적으로 변경
int main(int argc, char **argv) {
// argc : 인자의 갯수 , argv : 프로그램의 리스트
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
    port = argv[2];

    //host와 port로 서버를 찾아서 clientfd에 연결함
    clientfd = Open_clientfd(host, port); 
    //버퍼 rio 와  clientfd를 연결해줌
    Rio_readinitb(&rio, clientfd); 

    //stdin을 buf에 담음
    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        //buf에 담긴 값을 clientfd 에 옮겨씀
        Rio_writen(clientfd, buf, strlen(buf));
        //clientfd를 읽어서 buf에 옮김(송신)
        Rio_readlineb(&rio, buf, MAXLINE);
        //buf에 있는걸 stdout해줌(수신)
        Fputs(buf, stdout);
    }
    Close(clientfd);
    exit(0);

    return 0;
}