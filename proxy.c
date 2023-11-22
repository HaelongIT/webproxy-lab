#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

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

  if (strcasecmp(method, "GET") && strcasecmp(method,"HEAD")) {
    clienterror(clientfd, method, "501", "Not implemented",
                "구현되지 않음");
    return;
  }
  
  char host[256];
  char port[6];

  parse_uri(host, port, url);
  sprintf(headerbuf,"%s %s HTTP/1.0", method, url);
  
  printf("method: %s\n", method);
  printf("host:%s port:%s url:%s\n",host, port, url);
  printf("header: %s\n", headerbuf);
  fflush(stdout);

  if(port[0]=='\0'){
    strcpy(port,"5000");
  }

  serverfd = Open_clientfd(host, port);
  Rio_readinitb(&serverRio, serverfd);

  sprintf(headerbuf,"%s\nHost:%s\n"
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

int parse_uri(char *server_name, char *server_port, char *uri)
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

void clienterror(int fd, char *cause, char*errnum, 
                  char *shortmsg, char *longmsg)
{
    char buf[MAXBUF], body[MAXLINE];

    sprintf(body, "<html><head><title>error</title>");
    sprintf(body, "%s<meta charset='utf-8'></head>", body);
    sprintf(body, "%s<body bgcolor=\"ffffff\">\r\n", body); //fix
    sprintf(body, "%s<h1>%s: %s</h1>\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<h2>%s: %s</h2>\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>작은 웹 서버</em>\r\n",body);

    sprintf(buf,"HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html \r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

