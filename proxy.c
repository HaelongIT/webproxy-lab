#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
typedef struct cacheObject{
  struct cacheObject *prev;
  struct cacheObject *next;
  int size;
  char *uri;
  char *objectbuf;
}CacheObject;
typedef struct cache{
  int cache_size;
  CacheObject *header;
}Cache;

void doit(int , char *);
void read_requesthdrs(rio_t *, int, int *);
void *thread(void *);

void make_header(char *, char *, char *, char *);
int parse_uri(char *, char *, char *, char *);
int parse_handle(int, char *, char *, char *, char *);

void readRequest(int, char *, char *, char *);
void responseProxy(int, int, char *, Cache*);
void requestProxy(int, int, char *);
void clienterror(int , char *, char*, 
                  char *, char *); 

Cache *cachep;
CacheObject *cacheHeader;

int main(int argc, char **argv) {
  int listenfd, *connfdp;
  char hostname[MAX_CACHE_SIZE], port[MAX_CACHE_SIZE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  setCache();

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfdp = Malloc(sizeof(int));
    *connfdp = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAX_CACHE_SIZE, port, MAX_CACHE_SIZE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    Pthread_create(&tid, NULL, thread, connfdp);
  }
}

void setCache()
{
  cachep = (Cache *)Calloc(sizeof(Cache),1);
  cacheHeader = (CacheObject *)Calloc(sizeof(CacheObject),1);
  
  cacheHeader->uri="\0";
  cacheHeader->next = cacheHeader;
  cacheHeader->prev = cacheHeader;
  cacheHeader->size = 0;

  cachep->header = cacheHeader;

  return;
}

void *thread(void *vargp)
{
  int connfd = *((int *)vargp);
  Pthread_detach(pthread_self());
  Free(vargp);

  doit(connfd, cachep);
  Close(connfd);
  return NULL;  
}

void doit(int clientfd, char *cachep) {
  int is_static;
  struct stat sbuf;
  char cachebuf[MAX_CACHE_SIZE];
  char method[MAXLINE], url[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];

  char host[256], port[256], uri[MAXLINE];
  int serverfd;
  
  printf("\ndo it!\n");

  readRequest(clientfd, method, url, version); //첫줄 읽기(GET만)
  parse_uri(url, host, port, uri); //version 처리필요
  parse_handle(clientfd, method, host, port, uri);//구조체로 묶기

  int cacheFlag = 0;
  char headerbuf[MAXBUF];
  make_header(headerbuf, method, host, uri);
  cacheFlag = isCache(uri, cachep, cachebuf);
  if(cacheFlag){
    Rio_writen(clientfd, headerbuf, strlen(headerbuf)); //클라이언트 헤더 전송
    Rio_writen(clientfd, cachebuf, strlen(cachebuf)); //클라이언트로 전송
    printf("클라이언트에 전송\n");
  }else{
    serverfd = Open_clientfd(host, port);
    requestProxy(clientfd, serverfd, headerbuf);
    responseProxy(serverfd, clientfd, uri, cachep);
  }
}

CacheObject *makeCache(char* uri, char *buf){
  int size = strlen(buf);
  char* objbuf = (char *)Calloc(size,1);
  char* objuri = (char *)Calloc(strlen(uri),1);
  strcpy(objbuf,buf);
  strcpy(objuri,uri);
  CacheObject *cachePtr = (CacheObject *)Calloc(sizeof(CacheObject),1);
  cachePtr->objectbuf = objbuf;
  cachePtr->size = size;
  cachePtr->uri = objuri;
  return cachePtr;
}

int isCache(char *uri, Cache* cachep, char *buf){
  CacheObject *ptr = cachep->header->next;
  while(ptr->size!=0){
    if(!strcmp(uri,ptr->uri)){
      strcpy(buf,ptr->objectbuf);
      refreshCache(cachep, ptr);
      return 1;
    }
    ptr=ptr->next;
  }
  return 0;
}

void refreshCache(Cache *cachep, CacheObject *cachePtr){
  printf("캐쉬 새로고침: %s\n", cachePtr->uri);
  //최근 사용을 앞으로 당겨줌
  deleteCache(cachep, cachePtr);
  insertCacheToFirst(cachep, cachePtr);
}

void writeCache(Cache *cachep, CacheObject *cachePtr){
  printf("캐쉬 쓰기\n");
  while(cachep->cache_size+cachePtr->size>MAX_CACHE_SIZE)
  {
    printf("캐쉬 삭제 %d\n", cachep->cache_size);
    CacheObject *lastCache = cachep->header->prev;
    deleteCache(cachep, lastCache);
    Free(cachePtr->uri);
    Free(cachePtr->objectbuf);
    Free(cachePtr);
  }
  insertCacheToFirst(cachep, cachePtr);

}

void deleteCache(Cache *cachep, CacheObject *cachePtr){
  printf("캐쉬 분리: %s\n", cachePtr->uri);
  cachePtr->prev->next = cachePtr->next;
  cachePtr->next->prev = cachePtr->prev;
  cachep->cache_size-=cachePtr->size;
}

void insertCacheToFirst(Cache *cachep, CacheObject *cachePtr){
  CacheObject *header = cachep->header;
  //맨 앞에 삽입
  printf("맨 앞에 캐쉬 삽입: %s\n", cachePtr->uri);
  // printf("내용: %s\n", cachePtr->objectbuf);
  header->next->prev = cachePtr;
  cachePtr->next = header->next;
  header->next = cachePtr;
  cachePtr->prev = header;
  cachep->cache_size+=cachePtr->size;

}

void make_header(char* buf, char* method, char *host, char* uri){
  sprintf(buf,"%s %s HTTP/1.0", method, uri);
  sprintf(buf,"%s\nHost:%s\n"
  "User-Agent:%s\n"
  "Connection:close\n"
  "Proxy-Connection:close\r\n\r\n",buf,host,user_agent_hdr);
  printf("header: %s\n", buf);
}
//todo : robust하게 변경하기
int parse_uri(char *url, char *host, char *port, char *uri)
{

  char buf[MAXLINE];
  char *startp = strstr(url,"://");
  char *portDeli;
  char *uriDeli;

  if(startp){
    startp+=3;
  }else{
    startp=url;
  }
  strcpy(buf,startp);
  portDeli = index(buf,':');
  
  uriDeli = index(buf,'/');
  if(!uriDeli){
    strcat(uri,'/');
    uriDeli = buf+strlen(buf)-1;
  }

  if(!portDeli){
    strncpy(host, buf, uriDeli-buf);
    strcpy(port,"5000");
  }else{
    strncpy(host, buf, portDeli-buf);
    strncpy(port, portDeli+1, uriDeli-portDeli-1);
  }
  strcpy(uri, uriDeli);

}

int parse_handle(int clientfd, char *method, char *host, char *port, char *uri){
  printf("**method:%s \n host:%s \nport:%s uri:%s\n",method, host, port, uri);

  if (strcasecmp(method, "GET") && strcasecmp(method,"HEAD")) {
    clienterror(clientfd, method, "501", "Not implemented",
                "구현되지 않음");
    return -1;
  }

  return 0;
}

void readRequest(int clientfd, char *method, char *url, char *version){
  rio_t rio;
  char buf[MAX_CACHE_SIZE];

  Rio_readinitb(&rio, clientfd);
  Rio_readlineb(&rio, buf, MAX_CACHE_SIZE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, url, version);
}

void requestProxy(int clientfd, int serverfd, char *buf){
  Rio_writen(serverfd, buf, strlen(buf));
  printf("서버에 헤더 전송\n");
}

void responseProxy(int serverfd, int clientfd, char *uri, Cache *cachep){
  rio_t serverRio;
  char responsebuf[MAX_CACHE_SIZE];

  Rio_readinitb(&serverRio, serverfd); //서버 응답 읽음
  printf("서버응답 읽음\n");

  int content_length;
  // int *lenp = &content_length;
  // read_requesthdrs(&serverRio, clientfd, lenp);
  char buf[MAXBUF];
  printf("헤더 읽기 시작 \n");
  Rio_readlineb(&serverRio, buf, MAXBUF);
  Rio_writen(clientfd, buf, strlen(buf));
  while(strcmp(buf, "\r\n")){
    Rio_readlineb(&serverRio, buf, MAXBUF);
    printf("%s", buf);
    if (strstr(buf, "Content-Length")) // Response Body 수신에 사용하기 위해 Content-length 저장
      content_length = atoi(strchr(buf, ':') + 1);
    Rio_writen(clientfd, buf, strlen(buf));
  }
  printf("컨텐츠 길이:%d\n",content_length);

  Rio_readnb(&serverRio, responsebuf, MAX_CACHE_SIZE);
  Rio_writen(clientfd, responsebuf, content_length); //클라이언트로 전송
  printf("클라이언트에 전송\n");

  printf("buf len: %d Maxlen:%d\n", content_length, MAX_CACHE_SIZE);
  if(content_length <= MAX_CACHE_SIZE){
    CacheObject* newCache = makeCache(uri, responsebuf);
    writeCache(cachep, newCache);
  }
}

void read_requesthdrs(rio_t *rp, int clientfd, int* lenp)
{
  char buf[MAX_CACHE_SIZE];
  printf("헤더 읽기 시작 \n");
  Rio_readlineb(rp, buf, MAX_CACHE_SIZE);
  while(strcmp(buf, "\r\n")){
    Rio_readlineb(rp, buf, MAX_CACHE_SIZE);
    if (strstr(buf, "Content-Length")) // Response Body 수신에 사용하기 위해 Content-length 저장
      *lenp = atoi(strchr(buf, ':') + 1);
    printf("%s", buf);
    Rio_writen(clientfd, buf, strlen(buf));
  }
  
  printf("컨텐츠 길이:%d\n",*lenp);
  return;
}

void clienterror(int fd, char *cause, char*errnum, 
                  char *shortmsg, char *longmsg)
{
    char buf[MAXBUF], body[MAXBUF];

    sprintf(body, "<html><head><title>error</title>");
    sprintf(body, "%s<meta charset='utf-8'></head>", body);
    sprintf(body, "%s<body bgcolor=\"ffffff\">\r\n", body); //fix
    sprintf(body, "%s<h1>%s: %s</h1>\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<h2>%s: %s</h2>\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>작은 웹 서버</em>\r\n",body);

    sprintf(buf,"HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-Type: text/html \r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-Length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
