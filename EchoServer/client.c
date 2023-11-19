#include "csapp.h"
#include <stdio.h>


// 개인적으로 변경
int main(int argc, char **argv) {
// argc : 인자의 갯수 , argv : 프로그램의 리스트
    int clientfd;
    // 클라의 소켓(파일 디스크립션)

    char *host, *port, buf[MAXLINE];
    // 호스트번호, 포트번호, 버퍼

    rio_t rio;
    // 로버스트 I/O를 위한 구조체

    if (argc != 3) {
        fprintf(stderr, "usage: %s <호스트> <포트>\n", argv[0]);
        // argv[0] : 프로그램명
        // stderr에 출력
        exit(0);
    }
    // main함수를 실행할 때 3개의 인자가 들어오지 않으면 실행되는 곳

    host = argv[1];
    port = argv[2];
    // 호스트명과 포트명을 저장

    //host와 port로 서버를 찾아서 clientfd에 연결함
    clientfd = Open_clientfd(host, port); 
    // 서버의 호스트와 포트를 받아서 클라의 소켓을 열고 fd를 반환함

    //버퍼 rio 와  clientfd를 연결해줌
    Rio_readinitb(&rio, clientfd);
    // 해당 클라의 소켓을 rio에 초기화

    //stdin을 buf에 담음(stdin에 키보드로 입력된 걸 buf에 담음)
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
    // 전반적으로 그림 11.12와 주석이 있어서 미시적으로 따라갈수는 있는데
    // 거시적으로 이렇게 진행되어야만 클라이언트의 역할을 하는것이고 이런 부분은 책의 개념? 아니면 그냥 받아들이기?
    // 거시적으로 예상하면서 읽는게 아니고 그냥 흐름만 쫒아갈 뿐이다
}