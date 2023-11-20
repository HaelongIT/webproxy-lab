#include "csapp.h"
int main(void) {
  char *method, *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;

  method = getenv("REQUEST_METHOD");

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
  }

  sprintf(content, "<!DOCTYPE html><html lang=ko><head><meta charset=UTF-8><title>Adder</title></head><body><h1>So Dynamic</h1>");
  if(strcmp(buf,"")){
    sprintf(content,"%s<p>%d + %d = %d</p>",content,n1,n2,n1+n2);
  }
  sprintf(content,"%s<form action=\"/cgi-bin/adder\" method=\"get\">"
  "<input type=\"tex\" name=\"n1\" /><input type=\"text\" name=\"n2\" />"
  "<input type=\"submit\" value=\"Submit\" /></form>",content);
  sprintf(content,"%s</body></html>",content);

  /* Generate the HTTP response*/
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  if(strcmp(method,"HEAD")){
    printf("%s",content);
  }
  fflush(stdout);
  exit(0);
}

//todo printf->서버로
//todo template처럼 만들어보기