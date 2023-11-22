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
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);
void make_header(int fd, char *filename, int filesize, char* buf);
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

void doit(int fd) {
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  printf("tiny doit!\n");
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);

  if (strcasecmp(method, "GET") && strcasecmp(method,"HEAD")) {
    clienterror(fd, method, "501", "Not implemented\n",
                "구현되지 않음");
    return;
  }
  read_requesthdrs(&rio);

  is_static = parse_uri(uri, filename, cgiargs);
  if(stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not found",
                "작은 웹서버는 파일을 찾을 수 없어요\n");
    return;
  }

  if(is_static) {
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
                  "작은 웹서버가 읽을 수 없는 파일이네요!");
      return;
    }
    if(!strcasecmp(method, "HEAD")){
      make_header(fd, filename, sbuf.st_size, buf);     
    }else{
      serve_static(fd, filename, sbuf.st_size);
    }
  }
  else {
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
                  "작은 웹서버가 실행할 수 없는 파일이네요!");
      return;
    }
    serve_dynamic(fd, filename, cgiargs, method);
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

void make_header(int fd, char *filename, int filesize, char* buf){
  char filetype[MAXLINE];

  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%s서버: 작은 웹 서버\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
}

void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  make_header(fd,filename,filesize, buf);
  printf("Response headers:\n");
  printf("%s", buf);

  srcfd = Open(filename, O_RDONLY, 0);
  srcp = Mmap(0,filesize,PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  Munmap(srcp,filesize);
}

void get_filetype(char *filename, char *filetype)
{
  if(strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if(strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if(strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if(strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if(strstr(filename, ".mp4"))
    strcpy(filetype, "video/mpeg");
  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs, char *method)
{
  char buf[MAXBUF], *emptylist[] = {NULL};

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "서버: 다이나믹한 작은 웹서버\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if(Fork()==0){
    setenv("QUERY_STRING", cgiargs, 1);
    setenv("REQUEST_METHOD", method, 1);
    printf("\ncgiargs: %s\n", cgiargs);
    Dup2(fd, STDOUT_FILENO);
    Execve(filename, emptylist, environ);
  }
  Wait(NULL);
}
