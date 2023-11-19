#include "csapp.h"
int main(void) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;
  if ((buf = getenv("QUERY_STRING")) != NULL){
    p = strchr(buf,'&');
    *p = '\0';
    strcpy(arg1,buf);
    strcpy(arg2, p+1);
    n1 = atoi(arg1);
    n2 = atoi(arg2);
  }
  sprintf(content, "QUERY_STRING=%s",buf);
  sprintf(content, "<html><head><title>더하기</title>\r\n");
  sprintf(content, "%s<meta charset='utf-8'></head><body>\r\n",content);
  sprintf(content, "%s<h1>더하기 계산기입니다.</h1>\r\n",content);
  sprintf(content, "%s<h2>------------------------</h2>\r\n",content);
  sprintf(content, "%s<p>%d더하기 %d 는 %d 입니다.\r\n",content,n1,n2,n1+n2);
  sprintf(content, "%s<p>방문해 주셔서 감사합니다.\r\n",content);
  sprintf(content, "%s</body></html>\r\n",content);
  /* Generate the HTTP response*/
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s",content);
  fflush(stdout);
  exit(0);
}