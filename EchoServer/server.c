#include <stdio.h>
#include "csapp.h"

void echo(int connfd);

int main(int argc, char **argv) {
// argc : 인자의 갯수 , argv : 프로그램의 리스트

    int listenfd, connfd; //listen, conn 소켓 선언

    socklen_t clientlen; //소켓 길이 저장용

    struct sockaddr_storage clientaddr; //클라이언트 어드레스 저장용
    //반환받을 호스트네임, 포트네임

    char client_hostname[MAXLINE], client_port[MAXLINE];

    //에러처리. 
    if (argc != 2) {
        fprintf(stderr, "사용법: %s <포트>\n", argv[0]);
        exit(0);
    }

    //listen소켓 만들어줌
    listenfd = Open_listenfd(argv[1]);

    printf("리스닝 중 : %s \n", argv[1]);

    //연결을 계속 기다림
    while(1) {
        //소켓 구조체 크기 받음
        clientlen = sizeof(struct sockaddr_storage);
        
        //conn소켓 생성.  (clientaddr, clientlen 갱신)
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        //clientaddr로 호스트명, 포트명 갱신
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE,
                    client_port, MAXLINE, 0);
        printf("Connected to (%s %s)\n", client_hostname, client_port);
        echo(connfd); //연결 종료
        Close(connfd); //연결 종료
    }
    exit(0);

    // 미시적으로 흐름들은 따라가겠는데 이것들이 왜 거시적으로 서버의 역할은 하는건지는 어디서 알지? 책?
}