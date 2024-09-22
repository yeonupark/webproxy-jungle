/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
   /*
   buf: Query-String 환경변수를 담을 포인터
   p: 문자열을 파싱할 때 사용할 포인터
   arg1, arg2: 숫자를 저장할 버퍼
   content: 클라이언트에게 반환할 HTML 콘텐츠를 저장할 버퍼
   n1, n2: 숫자를 저장할 변수
   */
   char* buf, * p; 
   //char arg1[MAXLINE], arg2[MAXLINE];
   char content[MAXLINE];
   int n1 = 0, n2 = 0;

   //Extract the two arguments
   if ((buf = getenv("QUERY_STRING")) != NULL) {
      p = strchr(buf, '&');
      *p = '\0';
      // strcpy(arg1, buf);
      // strcpy(arg2, p + 1);
      sscanf(buf, "num1=%d", &n1);
      sscanf(p+1, "num2=%d", &n2);
      // n1 = atoi(arg1);
      // n2 = atoi(arg2);

   }

   //Make the response body
   printf("________%s\n", buf);
   sprintf(content, "QUERY_STRING=%s", buf);
   sprintf(content, "Welcome to add.com: ");
   sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
   sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>",
      content, n1, n2, n1 + n2);
   sprintf(content, "%sThanks for visiting!\r\n", content);

   //Generate the HTTP response
   printf("Connection: close\r\n");
   printf("Content-length: %d\r\n", (int)strlen(content));
   printf("Content-type: text/html\r\n\r\n");
   printf("%s", content);
   fflush(stdout); // 클라이언트로 즉시 출력이 전달되도록 출력 버퍼를 플러시

   exit(0);
}
/* $end adder */
