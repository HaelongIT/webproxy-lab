#include "csapp.h"
int main(void) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;
  // fprintf(0,"초기값 n1: %d n2: %d\n", n1, n2);

  if ((buf = getenv("QUERY_STRING")) != NULL){
    if(strcmp(buf,"")){
      p = strchr(buf,'&');
      *p = '\0';
      strcpy(arg1,buf);
      strcpy(arg2, p+1);
      p = index(arg1,'=');
      n1 = atoi(p+1);
      p = index(arg2,'=');
      n2 = atoi(p+1);
    }
    // fprintf(0,"\nn1: %d n2: %d\n", n1, n2);
  }


  sprintf(content, "<!DOCTYPE html><html lang=ko><head><meta charset=UTF-8><title>Adder</title></head><body><h1>So Dynamic</h1>");
  if(strcmp(buf,"")){
    sprintf(content,"%s<p>%d + %d = %d</p>",content,n1,n2,n1+n2);
  }
  sprintf(content,"%s<form action=\"/cgi-bin/adder\" method=\"get\">"
  "<input type=\"tex\" name=\"n1\" /><input type=\"text\" name=\"n2\" />"
  "<input type=\"submit\" value=\"Submit\" /></form>",content);
  sprintf(content,"%s</body></html>",content);
  // if(stat(filename, &sbuf) < 0) {
  //   clienterror(fd, filename, "404", "Not found",
  //               "작은 웹서버는 파일을 찾을 수 없어요");
  //   return;
  // }

  // int srcfd;
  // char * srcp;
  // srcfd = Open(filename, O_RDONLY, 0);
  // srcp = Mmap(0,filesize,PROT_READ, MAP_PRIVATE, srcfd, 0);
  // Close(srcfd);
  // // Rio_writen(fd, srcp, filesize);
  // printf("%s",srcp);
  // Munmap(srcp,filesize);

  
  /* Generate the HTTP response*/
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s",content);
  fflush(stdout);
  exit(0);
}