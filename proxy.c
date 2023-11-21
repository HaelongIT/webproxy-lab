#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */

void doit(int fd);
void read_requesthdrs(rio_t *);
void extract_host_port_and_suffix(const char *, char *, char *, char *);
void *thread(void *);

int main(int argc, char **argv) {
  int listenfd, *connfdp;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfdp = Malloc(sizeof(int));
    *connfdp = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    Pthread_create(&tid, NULL, thread, connfdp);
  }
}

void *thread(void *vargp)
{
  int connfd = *((int *)vargp);
  Pthread_detach(pthread_self());
  Free(vargp);
  doit(connfd);
  Close(connfd);
  return NULL;  
}

void doit(int clientfd) {
  int is_static;
  struct stat sbuf;
  char startbuf[MAXLINE], clientbuf[MAXLINE], responsebuf[MAXLINE], headerbuf[MAXLINE];
  char method[MAXLINE], url[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio, serverRio;

  int serverfd;
  
  printf("\ndoit!\n");

  Rio_readinitb(&rio, clientfd);
  Rio_readlineb(&rio, startbuf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", startbuf);
  sscanf(startbuf, "%s %s %s", method, url, version);
  
  char host[256];
  char port[6];

  parse_uri(host, port, url, filename, cgiargs);
  sprintf(headerbuf,"%s %s HTTP/1.0", method, filename);
  
  printf("method: %s\n", method);
  printf("url:%s host:%s port:%s filename: %s\n",url, host, port, filename);
  printf("header: %s\n", headerbuf);
  fflush(stdout);

  // if(port[0]=='\0'){
  //   strcpy(port,"5000");
  // }

  serverfd = Open_clientfd(host, port);
  Rio_readinitb(&serverRio, serverfd);

  sprintf(headerbuf,"%sHost:%s\n"
  "User-Agent:%s\n"
  "Connection:close\n"
  "Proxy-Connection:close\r\n\r\n",headerbuf,host,user_agent_hdr);
  Rio_writen(serverfd, headerbuf, strlen(headerbuf));
  printf("서버에 헤더 전송\n");
  // Rio_writen(serverfd, clientbuf, strlen(clientbuf)); //서버에 전송
 
  Rio_readnb(&serverRio, responsebuf, MAXLINE); //서버 응답 읽음
  printf("서버응답 읽음\n");
  Rio_writen(clientfd, responsebuf, strlen(responsebuf)); //클라이언트로 전송
  printf("클라이언트에 전송\n");
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXBUF];
  printf("헤더 읽기 시작 \n");
  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")){
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *server_name, char *server_port, char *uri, char *filename, char *cgiargs)
{
    // http://localhost:9999/cgi-bin/adder?123&456
    char parsed_uri[MAXLINE]={0};
    
    char *parser_ptr = strstr(uri, "//") ? strstr(uri, "//") + 2 : uri;

    int i=0;
    // int length=strlen(*uri);
    // int cnt=0;

    while(*parser_ptr!=':'){
        server_name[i]=*parser_ptr;
        i++;
        // cnt++;
        parser_ptr++;
    }
    i=0;
    // cnt++;
    parser_ptr++;
    while(*parser_ptr!='/'){
        server_port[i]=*parser_ptr;
        i++;
        // cnt++;
        parser_ptr++;
    }
    i=0;
    while(*parser_ptr){
        parsed_uri[i]=*parser_ptr;
        i++;
        // cnt++;        
        parser_ptr++;
    }

    strcpy(uri,parsed_uri);

    return 0;
}


void extract_host_port_and_suffix(const char *url, char *host, char *port, char *suffix) {
    // 기본 값 설정
    strcpy(port, "");
    strcpy(suffix, "");

    // URL에서 프로토콜 부분을 찾아 넘깁니다.
    const char *host_start = strstr(url, "://");
    if (host_start) {
        host_start += 3;
    }else{
        host_start = url;
    }

    // 호스트 부분의 끝을 찾습니다.
    const char *host_end = strpbrk(host_start, "/:");
    if (!host_end) {
        host_end = url + strlen(url); // URL의 끝을 호스트 끝으로 설정
    }
    
    size_t host_len = host_end - host_start;
    strncpy(host, host_start, host_len);
    host[host_len] = '\0'; // 문자열 종료를 위해 널 문자를 추가합니다.

    // 포트 번호가 있는 경우 추출합니다.
    if (*host_end == ':') {
        host_end++; // 포트 번호 시작 위치로 이동
        const char *port_end = strpbrk(host_end, "/"); // 포트 번호 끝을 찾습니다.
        if (!port_end) {
            port_end = url + strlen(url); // URL의 끝을 포트 끝으로 설정
        }
        strncpy(port, host_end, port_end - host_end);
        port[port_end - host_end] = '\0'; // 문자열 종료를 위해 널 문자를 추가합니다.
        host_end = port_end;
    }

    // URL 접미사를 복사합니다.
    if (*host_end != '\0') {
        strcpy(suffix, host_end);
    }
}
