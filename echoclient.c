#include "csapp.h"

int main(int argc, char **argv) {
    int clientfd; // 클라이언트 소켓 파일 디스크립터
    char *host, *port, buf[MAXLINE]; // 호스트 이름, 포트 번호, 데이터 버퍼
    rio_t rio; // Robust I/O 구조체

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port); // 클라이언트 소켓 열고 지정된 호스트와 포트로 연결
    Rio_readinitb(&rio, clientfd); // Robust I/O를 사용하기 위해 소켓 파일 디스크립터를 초기화

    // 표준 입력(stdin)에서 입력을 받아 서버에 전송하는 루프
    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        Rio_writen(clientfd, buf, strlen(buf));  // 표준 입력에서 입력받은 데이터를 서버에 전송
        Rio_readlineb(&rio, buf, MAXLINE); // 서버의 응답을 받아 buf에 저장 (한 줄씩 읽음)
        Fputs(buf, stdout); // 서버로부터 받은 데이터를 표준 출력 (stdout)에 출력
    }
    Close(clientfd);
    exit(0);
}