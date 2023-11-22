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
void read_requesthdrs(rio_t *);
void *thread(void *);

void make_header(char *, char *, char *, char *);
int parse_uri(char *, char *, char *);
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
  char hostname[MAXLINE], port[MAXLINE];
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
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
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
  char cachebuf[MAXLINE];
  char method[MAXLINE], url[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];

  char host[256];
  char port[6];
  int serverfd;
  
  printf("\ndo it!\n");

  readRequest(clientfd, method, url, version); //첫줄 읽기(GET만)
  parse_uri(host, port, url); //version 처리필요
  parse_handle(clientfd, method, host, port, url);//구조체로 묶기

  int cacheFlag;
  cacheFlag = isCache(url, cachep, cachebuf);
  if(cacheFlag){
    Rio_writen(clientfd, cachebuf, strlen(cachebuf)); //클라이언트로 전송
    printf("클라이언트에 전송\n");
  }else{
    serverfd = Open_clientfd(host, port);
    char headerbuf[MAXLINE];
    make_header(headerbuf, method, host, url);
    requestProxy(clientfd, serverfd, headerbuf);
    responseProxy(serverfd, clientfd, url, cachep);

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

int parse_handle(int clientfd, char *method, char *host, char *port, char *url){
  printf("method: %s\n", method);
  printf("host:%s port:%s url:%s\n",host, port, url);

  if (strcasecmp(method, "GET") && strcasecmp(method,"HEAD")) {
    clienterror(clientfd, method, "501", "Not implemented",
                "구현되지 않음");
    return -1;
  }
  //자동 포트 배정
  if(port[0]=='\0'){
    strcpy(port,"5000");
  }

  return 0;
}

void readRequest(int clientfd, char *method, char *url, char *version){
  rio_t rio;
  char buf[MAXLINE];

  Rio_readinitb(&rio, clientfd);
  Rio_readlineb(&rio, buf, MAXLINE);
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
  char responsebuf[MAXLINE];

  Rio_readinitb(&serverRio, serverfd);
  Rio_readnb(&serverRio, responsebuf, MAXLINE); //서버 응답 읽음
  printf("서버응답 읽음\n");
  
  Rio_writen(clientfd, responsebuf, strlen(responsebuf)); //클라이언트로 전송
  printf("클라이언트에 전송\n");

  strcpy(responsebuf, strstr(responsebuf,"\r\n\r\n")+4);

  printf("buf len: %d Maxlen:%d\n", strlen(responsebuf), MAX_OBJECT_SIZE);
  if(strlen(responsebuf)<=MAX_OBJECT_SIZE){
    CacheObject* newCache = makeCache(uri, responsebuf);
    writeCache(cachep, newCache);
  }
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
//todo : robust하게 변경하기
int parse_uri(char *server_name, char *server_port, char *uri)
{
    char parsed_uri[MAXLINE]={0};
    
    char *parser_ptr = (uri, "//") ? strstr(uri, "//") + 2 : uri;

    int i=0;

    while(*parser_ptr!=':'){
        server_name[i]=*parser_ptr;
        i++;
        parser_ptr++;
    }
    i=0;
    parser_ptr++;
    while(*parser_ptr!='/'){
        server_port[i]=*parser_ptr;
        i++;
        parser_ptr++;
    }
    i=0;
    while(*parser_ptr){
        parsed_uri[i]=*parser_ptr;
        i++;     
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
