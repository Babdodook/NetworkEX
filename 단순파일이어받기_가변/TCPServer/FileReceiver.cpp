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

// 현재 디렉토리에서 파일 존재 여부를 확인한다.
BOOL SearchFile(char *filename)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFindFile = FindFirstFile(filename, &FindFileData);
	if(hFindFile == INVALID_HANDLE_VALUE)
		return FALSE;
	else{
		FindClose(hFindFile);
		return TRUE;
	}
}

int main(int argc, char* argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return -1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == INVALID_SOCKET) ErrorHandling("socket()");	
	
	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) ErrorHandling("connect()");

	
		// 파일 이름 받기
		char filename[256];
		ZeroMemory(filename, 256);
		retval = recvn(sock, filename, 256, 0);
		if(retval == SOCKET_ERROR){
			ErrorHandling ("recv()");
			}
		printf("-> 받을 파일 이름: %s\n", filename);

		// 파일 검색
		int currbytes = 0;
		if(SearchFile(filename)){
			// 현재의 파일 크기 얻기
			FILE *fp = fopen(filename, "rb");
			if(fp == NULL){
				ErrorHandling("파일 입출력 오류");
				}
			fseek(fp, 0, SEEK_END);
			currbytes = ftell(fp);
			fclose(fp);
		}

		// 전송 시작할 위치(=현재의 파일 크기) 보내기
		retval = send(sock, (char *)&currbytes, sizeof(currbytes), 0);
		if(retval == SOCKET_ERROR){
			ErrorHandling("send()");
			}			

		// 전송 받을 데이터 크기 받기
		int totalbytes;
		retval = recvn(sock, (char *)&totalbytes, sizeof(totalbytes), 0);
		if(retval == SOCKET_ERROR){
			ErrorHandling("recv()");
			}
		printf("-> 받을 데이터 크기: %d\n", totalbytes);

		// 파일 열기
		FILE *fp = fopen(filename, "ab"); // append & binary mode
		if(fp == NULL){
			ErrorHandling("파일 입출력 오류");
		}

		// 파일 데이터 받기
		int numtotal = 0;
		char buf[BUFSIZE];
		while(1){
			retval = recvn(sock, buf, BUFSIZE, 0);
			if(retval == SOCKET_ERROR){
				ErrorHandling("recv()");
				}
			else if(retval == 0) 
				break;
			else{
				fwrite(buf, 1, retval, fp);
				if(ferror(fp)){
					ErrorHandling("파일 입출력 오류");
					}
				numtotal += retval;
			}
		}
		fclose(fp);

			// closesocket()
		closesocket(sock);
	
	
	// 윈속 종료
	WSACleanup();
	return 0;
}