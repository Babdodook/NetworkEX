#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib,"ws2_32")
#include<WinSock2.h>
#include<stdlib.h>
#include<stdio.h>

#define SERVERIP "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 512

void err_quit(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, msg, (LPCSTR)lpMsgBuf, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

void err_display(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

int recvn(SOCKET s, char* buf, int len, int flag)
{
	int received;
	char* ptr = buf;
	int left = len;

	while (left > 0)
	{
		received = recv(s, ptr, len, flag);
		if (received == SOCKET_ERROR)
		{
			return SOCKET_ERROR;
		}
		else if (received == 0)
			break;

		left -= received;
		ptr += received;
	}

	return (len - left);
}

typedef struct User {
	char id[10];
	char pw[10];
}User;

int main(int argc, char *argv[])
{
	int retval;

	User root;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");

	char buf[BUFSIZE + 1];
	int len;

	while (1)
	{
		retval = recv(sock, buf, strlen(buf), 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recvn()");
			break;
		}
		else if (retval == 0)
			break;

		buf[retval] = '\0';
		printf("\n[TCP 클라이언트] %s ", buf);

		scanf("%s", root.id);
		scanf("%s", root.pw);

		int len;
		len = sizeof(root);

		//구조체 크기
		retval = send(sock, (char*)&len, sizeof(int), 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
			break;
		}

		//데이터 보냄
		retval = send(sock, (char*)&root, sizeof(User), 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
			break;
		}

		//결과 데이터 받기
		retval = recv(sock, buf, strlen(buf), 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recvn()");
			break;
		}
		else if (retval == 0)
			break;

		buf[retval] = '\0';
		printf("\n[TCP 클라이언트] %s ", buf);
	}
	closesocket(sock);

	WSACleanup();
	return 0;
}