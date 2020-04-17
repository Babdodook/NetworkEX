#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define BUFSIZE 4096

// 소켓 함수 오류 출력 후 종료
void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

// 사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while(left > 0){
		received = recv(s, ptr, left, flags);
		if(received == SOCKET_ERROR) 
			return SOCKET_ERROR;
		else if(received == 0) 
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

int main(int argc, char* argv[])
{
	int retval;

	if(argc < 2){
		printf("Usage: %s [FileName]\n", argv[0]);
		return -1;
	}

	// 윈속 초기화
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return -1;

	// socket()
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_sock == INVALID_SOCKET) ErrorHandling("socket()");	
	
	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) ErrorHandling("bind()");
	
	// listen()
	retval = listen(listen_sock, 5);
	if(retval == SOCKET_ERROR) ErrorHandling("listen()");

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE];

	while(1){
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
		if(client_sock == INVALID_SOCKET){
			ErrorHandling ("accept()");
		}
		printf("\nFileSender 접속: IP 주소=%s, 포트 번호=%d\n", 
			inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);

	// 파일 열기
	FILE *fp = fopen("VMware-workstation-6.0.0-45731.exe", "rb");
	if(fp == NULL){
		ErrorHandling("파일 입출력 오류");
		return -1;
	}
	
	// 파일 이름 보내기
	char filename[256];
	ZeroMemory(filename, 256);
	sprintf(filename, "VMware-workstation-6.0.0-45731.exe");
	retval = send(client_sock, filename, 256, 0);
	if(retval == SOCKET_ERROR) ErrorHandling("send()");	

	// 전송 시작할 위치(=현재의 파일 크기) 받기
	int currbytes;
	retval = recvn(client_sock, (char *)&currbytes, sizeof(currbytes), 0);
	if(retval == SOCKET_ERROR) ErrorHandling("recv()");

	// 파일 크기 얻기
	fseek(fp, 0, SEEK_END);
	int totalbytes = ftell(fp);
	totalbytes -= currbytes;
	
	// 전송할 데이터 크기 보내기
	retval = send(client_sock, (char *)&totalbytes, sizeof(totalbytes), 0);
	if(retval == SOCKET_ERROR) ErrorHandling("send()");	

	// 파일 데이터 전송에 사용할 변수
	char buf[BUFSIZE];
	int numread;
	int numtotal = 0;

	// 파일 데이터 보내기
	fseek(fp, currbytes, SEEK_SET); // 파일 포인터를 전송 시작 위치로 이동
	while(1){
		numread = fread(buf, 1, BUFSIZE, fp);
		if(numread > 0){
			retval = send(client_sock, buf, numread, 0);
			if(retval == SOCKET_ERROR){
				ErrorHandling("send()");
			}
			numtotal += numread;
			printf("."); // 전송 상황을 표시
			Sleep(50); // 전송 중단 실험을 위해 속도를 느리게 함
		}
		else if(numread == 0 && numtotal == totalbytes){
			printf("\n파일 전송 완료!: %d 바이트\n", numtotal);
			break;
		}
		else{
			ErrorHandling("파일 입출력 오류");
		}
	}
	fclose(fp);
    closesocket(client_sock);
		printf("FileSender 종료: IP 주소=%s, 포트 번호=%d\n", 
		inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
	}
	
	closesocket(listen_sock);
	

	// 윈속 종료
	WSACleanup();
	return 0;
}