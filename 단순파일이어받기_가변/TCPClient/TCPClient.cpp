#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 4096

enum PROTOCOL {
	NONEFILE = 1,
	FILEEXIST,
	FILELACK,
	SENDFILE,
	END
};

// 소켓 함수 오류 출력 후 종료
void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
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

bool PacketRecv(SOCKET _sock, char* buf)
{
	int size;

	int retval = recvn(_sock, (char*)& size,
		sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recv()");
		return false;
	}
	else if (retval == 0)
	{
		return false;
	}

	retval = recvn(_sock, buf, size, 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recv()");
		return false;
	}
	else if (retval == 0)
	{
		return false;
	}

	return true;
}

int PackPacket(char *buf, PROTOCOL protocol, char *str, int bytes)
{
	int size = 0;
	int strsize = strlen(str);
	char* ptr = buf + sizeof(int);

	memcpy(ptr, &protocol, sizeof(PROTOCOL));
	size += sizeof(PROTOCOL);
	ptr += sizeof(PROTOCOL);

	memcpy(ptr, &strsize, sizeof(int));
	size += sizeof(int);
	ptr += sizeof(int);

	memcpy(ptr, str, strsize);
	size += strsize;
	ptr += strsize;

	memcpy(ptr, &bytes, sizeof(int));
	size += sizeof(int);

	ptr = buf;
	memcpy(ptr, &size, sizeof(int));

	return size + sizeof(int);
}

int PackPacket(char* buf, PROTOCOL protocol, char* file, int bytes, int set)
{
	int size = 0;
	char* ptr = buf + sizeof(int);

	memcpy(ptr, &protocol, sizeof(PROTOCOL));
	size += sizeof(PROTOCOL);
	ptr += sizeof(PROTOCOL);

	memcpy(ptr, &bytes, sizeof(int));
	size += sizeof(int);
	ptr += sizeof(int);

	memcpy(ptr, file, bytes);
	size += bytes;

	ptr = buf;
	memcpy(ptr, &size, sizeof(int));

	return size + sizeof(int);
}

void UnPackPacket(char* buf, int *bytes)
{
	const char* ptr = buf + sizeof(PROTOCOL);

	memcpy(bytes, ptr, sizeof(int));
}

PROTOCOL GetProtocol(char* buf)
{
	PROTOCOL protocol;
	char* ptr = buf;
	memcpy(&protocol, ptr, sizeof(PROTOCOL));

	return protocol;
}

int main(int argc, char *argv[])
{
	int retval;

	if (argc < 2) 
	{
		printf("Usage: %s [FileName]\n", argv[0]);
		return -1;
	}
	// 윈속 초기화
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("connect()");

	// 데이터 통신에 사용할 변수
	char buf[5000];
	
	FILE *fp = fopen(argv[1], "rb");
	if (fp == NULL) {
		err_display("파일 입출력 오류");
		return -1;
	}

	// 서버와 데이터 통신
	char filename[256];
	ZeroMemory(filename, 256);
	sprintf(filename, argv[1]);

	// 파일 크기 얻기
	fseek(fp, 0, SEEK_END);
	int totalbytes = ftell(fp);

	int size = PackPacket(buf, NONEFILE, filename, totalbytes);		// 파일 이름, 파일의 총 크기 팩킹

	retval = send(sock, buf, size, 0);	// filename, 총 크기 전송
	if (retval == SOCKET_ERROR) err_display("send()");

	// 전송 시작할 위치(=현재의 파일 크기) 받기
	if (!PacketRecv(sock, buf))
	{
		err_display("패킷 받기 오류");
		return -1;
	}

	int currbytes = 0;
	PROTOCOL protocol = GetProtocol(buf);
	

	switch (protocol)
	{
	case NONEFILE:
		printf("새로운 파일을 전송합니다.\n");
		break;
	case FILELACK:
		printf("이어서 파일을 전송합니다.\n");
		UnPackPacket(buf, &currbytes);
		break;
	case FILEEXIST:
		printf("이미 전송된 파일입니다.\n");
		exit(1);
	}

	// 파일 데이터 전송에 사용할 변수
	
	int numread;
	int numtotal = 0;

	// 파일 데이터 보내기
	fseek(fp, currbytes, SEEK_SET); // 파일 포인터를 전송 시작 위치로 이동

	char file[BUFSIZE];
	ZeroMemory(file, BUFSIZE);
	while (1)
	{
		numread = fread(file, 1, BUFSIZE, fp);
		if (numread > 0) {
			size = PackPacket(buf, SENDFILE, file, numread, 0);

			retval = send(sock, buf, size, 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
			}

			numtotal += numread;
			printf(".."); // 전송 상황을 표시
			Sleep(500);  // 전송 중단 실험을 위해 속도를 느리게 함
		}
		else if (numread == 0 && numtotal == totalbytes) {
			printf("\n파일 전송 완료!: %d 바이트\n", numtotal);
			break;
		}
		else {			 // 파일 다 보낸 경우
			size = PackPacket(buf, END, file, numread, 0);

			retval = send(sock, buf, size, 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
			}
			break;
		}
	}

	fclose(fp);
	closesocket(sock);
	// 윈속 종료
	WSACleanup();
	return 0;
}