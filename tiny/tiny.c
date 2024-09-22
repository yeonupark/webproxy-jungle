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

/* open a listening socket, and then repeatedly accept a connection request, perform a transation, close end of the connection in a loop */
int main(int argc, char **argv) {

  signal(SIGPIPE, SIG_IGN);

  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command-line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); // opening a listening socket
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // acceptig a connection request
    Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0); // converting socketaddr to form that is available to read
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd); // performing a transaction
    Close(connfd); // closing its end of the connenction
  }
}

/* handle one HTTP transaction. read and parse the request line and figure out whether it is static or dynamic content */
void doit(int fd) {

  int is_static;
  struct stat sbuf; // 파일의 정보(메타 데이터)를 저장하는 구조체
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; // 클라이언트가 보낸 요청 라인의 각 요소를 저장할 버퍼
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio; // Robust I/O: 버퍼링된 입출력 처리

  /* Read Request line and headers */
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE); // 클라이언트로부터 요청 라인을 읽어 buf에 저장

  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); // 요청 라인 파싱. 각각 저장
  if (strcasecmp(method, "GET" )) { // GET이나 HEAD가 아닌 요청일 때 에러 처리 (homework 11.11)
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio); // 요청 헤더 읽기
  
  /* Parse URI from GET request */
  // 정적 콘텐츠: HTML 파일이나 이미지 같은 서버에 저장된 파일을 직접 제공하는 콘텐츠
  // 동적 콘텐츠: CGI 프로그램을 통해 생성된 콘텐츠
  is_static = parse_uri(uri, filename, cgiargs); // 1이면 정적, 0이면 동적
  if (stat(filename, &sbuf) < 0) { // 파일 존재 여부 확인. (stat: filename에 해당하는 파일의 정보를 가져와 sbuf에 저장)
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  if (is_static) {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { // 요청한 파일이 정상적이고 읽기 권한을 가지고 있는지
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);
  } else {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}

// send an HTTP response to the client with the appropriate status code and status message
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

/* Read and ignore request headers */
void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n")) { // strcmp: 일치하면 0 반환
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

/* Parse an HHTP URI */
int parse_uri(char *uri, char *filename, char *cgiargs) {
  char *ptr;
  
  if (!strstr(uri, "cgi-bin")) { // static content
    strcpy(cgiargs, ""); // clear the CGI argument string
    strcpy(filename, "."); 
    strcat(filename, uri); // convert the URI into a relative Linux pathname such as ./index.html
    if (uri[strlen(uri)-1] == '/') // if the URL ends with '/'
      strcat(filename, "home.html"); // append the default file name
    return 1;
  } else { // dynamic content
    ptr = strstr(uri, "?"); // extract any CGI arguments. uri내 ?의 첫 위치 저장
    if (ptr) {
      strcpy(cgiargs, ptr); 
      *(ptr) = '\0';
    }
    else 
      strcpy(cgiargs, "");
    strcpy(filename, "."); // convert the remaining portion of the URI to a relative Linux filename
    strcat(filename, uri); 
    return 0;
  }
}

/* serve static content to a client */
void serve_static(int fd, char *filename, int filesize) { 
  /*
  srcfd: 파일을 열 때 사용되는 파일 디스크립터
  scrp: 파일 내용을 메모리에 매핑하여 그 메모리 주소를 가리키는 포인터
  filetype: 파일의 MIME 타입을 저장하는 문자열
  buf: 클라이언트에 전송할 HTTP 응답 헤더를 담는 버퍼 
  */
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* send response headers to client */
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n", buf);
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf)); // 헤더 전송: 생성된 응답 헤더를 클라이언트에 전송함. 버퍼의 데이터를 클라이언트 소켓에 쓴다는 의미의 함수
  printf("Response headers:\n");
  printf("%s", buf);

  /* send response body to client */
  srcfd = Open(filename, O_RDONLY, 0); // 파일 열기 (읽기 전용으로)
  /*
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 메모리 매핑 -> 파일의 내용을 메모리 공간에 올려서 파일을 읽을 때 메모리에서 읽는 것 처럼 처리
  Close(srcfd); // 파일 디스크립터 닫기 (파일을 메모리에 매핑했으므로 파일 디스크립터는 더 이상 필요하지 않음)
  Rio_writen(fd, srcp, filesize); // 파일 내용을 클라이언트에 전송: 매핑된 메모리 주소에 있는 내용을 클라이언트 소켓(fd)를 통해 전송
  Munmap(srcp, filesize); // 메모리 매핑 해제
  */

 // 연습 문제 11.9 ver. (mmap 대신에 malloc 쓰기)
  srcp = (char *) malloc(filesize); // 파일을 저장할 공간의 메모리를 할당
  rio_readn(srcfd, srcp, filesize); // 파일의 내용을 읽어 할당된 메모리에 저장함
  Close(srcfd);
  Rio_writen(fd, srcp, filesize); // 할당된 메모리에 저장된 파일 내용을 클라이언트에게 전송
  free(srcp);
}

/* derive file type from filename */
void get_filetype(char *filename, char *filetype) {
  if (strstr(filename, ".html"))
    strcpy(filetype, "text.html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else 
    strcpy(filetype, "image/png");
}

/* serve dynamic content to a client */
void serve_dynamic(int fd, char *filename, char *cgiargs) {
  char buf[MAXLINE], *emptylist[] = { NULL };

  /* Return first part of HTTP response */                                                                                                                                                                                          
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0) { /* Child */
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1); // the child initializes the Q_S environment variable with the CGI arguments from the request URI
    Dup2(fd, STDOUT_FILENO); // redirect stdout to client
    Execve(filename, emptylist, environ); // load and run the CGI program (in the context of the child)
  }
  Wait(NULL); // parent waits for and reaps child when it terminates
}
