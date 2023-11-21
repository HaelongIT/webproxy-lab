#include "csapp.h"

char * replace(
    char const * const original, 
    char const * const pattern, 
    char const * const replacement
);

int main(void) {
  char *method, *filebuf, *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;

  method = getenv("REQUEST_METHOD");

  if ((buf = getenv("QUERY_STRING")) != NULL){
    if(buf[0]!='\0'){
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

  content[0] = '\0';
  char num[100];
  char *temp;
  int srcfd;
  srcfd = Open("./cgi-bin/adder.html", O_RDONLY, 0);
  filebuf = (char *)malloc(MAXLINE);
  Rio_readn(srcfd,filebuf,MAXLINE);
  Close(srcfd);

  strcpy(content, filebuf);
  free(filebuf);

  //첫 번째 replace_substr 호출
  sprintf(num, "%d", n1);
  temp = replace(content, &"{@n1}", num);
  strcpy(content, temp);
  free(temp);

  sprintf(num, "%d", n2);
  temp = replace(content, &"{@n2}", num);
  strcpy(content, temp);
  free(temp);

  sprintf(num, "%d", n1+n2);
  temp = replace(content, &"{@n3}", num);
  strcpy(content, temp);
  free(temp);

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

char * replace(
    char const * const original, 
    char const * const pattern, 
    char const * const replacement
) {
  size_t const replen = strlen(replacement);
  size_t const patlen = strlen(pattern);
  size_t const orilen = strlen(original);

  size_t patcnt = 0;
  const char * oriptr;
  const char * patloc;

  // find how many times the pattern occurs in the original string
  for (oriptr = original; patloc = strstr(oriptr, pattern); oriptr = patloc + patlen)
  {
    patcnt++;
  }

  {
    // allocate memory for the new string
    size_t const retlen = orilen + patcnt * (replen - patlen);
    char * const returned = (char *) malloc( sizeof(char) * (retlen + 1) );

    if (returned != NULL)
    {
      // copy the original string, 
      // replacing all the instances of the pattern
      char * retptr = returned;
      for (oriptr = original; patloc = strstr(oriptr, pattern); oriptr = patloc + patlen)
      {
        size_t const skplen = patloc - oriptr;
        // copy the section until the occurence of the pattern
        strncpy(retptr, oriptr, skplen);
        retptr += skplen;
        // copy the replacement 
        strncpy(retptr, replacement, replen);
        retptr += replen;
      }
      // copy the rest of the string.
      strcpy(retptr, oriptr);
    }
    return returned;
  }
}
