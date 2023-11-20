/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
// main 함수의 인자 갯수, 인자들
  int listenfd, connfd;
  // 듣기소켓, 연결소켓

  char hostname[MAXLINE], port[MAXLINE];
  // 호스트명, 포트번호

  socklen_t clientlen;
  // 클라의 길이

  struct sockaddr_storage clientaddr;
  // 클라의 주소

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  // main함수를 실행할 때 인자로 포트번호가 들어오지 않으면 오류처리

  listenfd = Open_listenfd(argv[1]);
  // 리슨소켓 만들기

  // 연결을 기달림
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept

    //clientaddr로 호스트명, 포트명 갱신
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    
    doit(connfd);   // line:netp:tiny:doit
    // 에코서버와 다른점

    Close(connfd);  // line:netp:tiny:close
  }
}

void doit(int fd) {
  int is_static;
  // 정적인지 동적인지 확인

  struct stat sbuf;

  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  Rio_readinitb(&rio, fd);
  // 요청라인을 rio에 초기화

  Rio_readlineb(&rio, buf, MAXLINE);
  // 요청라인을 읽어서 buf에 옮김

  printf("Request headers:\n");
  printf("%s", buf);

  sscanf(buf, "%s %s %s", method, uri, version);
  // 버퍼(요청라인)를 읽어와서 저장함

  if (strcasecmp(method, "GET")) {
  // get요청이 아닌경우

    clienterror(fd, method, "501", "Not implemented",
                "구현되지 않음");
    // 에러처리
    return;
  }

  read_requesthdrs(&rio);
  // 요청라인을 읽고 무시한다

  is_static = parse_uri(uri, filename, cgiargs);
  // 정적 컨텐츠인지 확인

  if(stat(filename, &sbuf) < 0) {
  // 파일의 속성을 읽어서 실패했으면(디스크 상에 없으면)

    clienterror(fd, filename, "404", "Not found",
                "작은 웹서버는 파일을 찾을 수 없어요");
    // 에러처리
    return;
  }

  if(is_static) {
  // 정적컨텐츠 일때
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
    // 보통파일인지랑 읽기권한 확인
      clienterror(fd, filename, "403", "Forbidden",
                  "작은 웹서버가 읽을 수 없는 파일이네요!");
      // 에러처리
      return;
    }
    serve_static(fd, filename, sbuf.st_size);
    // 정적인 컨텐츠 클라에게 제공
  }
  else {
  // 동적컨텐츠일 경우
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
                  "작은 웹서버가 실행할 수 없는 파일이네요!");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
    // 동적인 컨텐츠 클라에게 제공
  }
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

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXBUF];

  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")){
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *uri, char *filename, char*cgiargs)
{
  char *ptr;

  if(!strstr(uri, "cgi-bin")){
    strcpy(cgiargs , "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if( uri[strlen(uri)-1]== '/')
      strcat(filename, "home.html");
    return 1;
  }
  else {
    ptr = index(uri, '?');
    if(ptr){
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs,"");
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%s서버: 작은 웹 서버\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  srcfd = Open(filename, O_RDONLY, 0);

  // srcp = Mmap(0,filesize,PROT_READ, MAP_PRIVATE, srcfd, 0);
  // Close(srcfd);
  // Rio_writen(fd, srcp, filesize);
  // Munmap(srcp,filesize);

  // 11.9를 위해 추가
  srcp = (char *)malloc(sizeof(filesize));
  Rio_readn(srcfd, srcp, filesize);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  free(srcp);
}

void get_filetype(char *filename, char *filetype)
{
  if(strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if(strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if(strstr(filename, "./png"))
    strcpy(filetype, "image/png");
  else if(strstr(filename, "./jpg"))
    strcpy(filetype, "image/jpeg");  //jpeg는?
  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXBUF], *emptylist[] = {NULL};

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "서버: 다이나믹한 작은 웹서버\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if(Fork()==0){
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);
    Execve(filename, emptylist, environ);
  }
  Wait(NULL);
}