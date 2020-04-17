#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib,"ws2_32")
#include<WinSock2.h>
#include<stdlib.h>
#include<stdio.h>
#include<conio.h>

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
	strcpy(root.id, "root");
	strcpy(root.pw, "root");

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	if (listen(listen_sock, SOMAXCONN))
		err_quit("listen()");

	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE + 1];

	while (1)
	{
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			err_display("accept()");
			break;
		}

		printf("\n[TCP 서버] 클라이언트 접속: IP=%s, 포트 번호=%d\n",
			inet_ntoa(clientaddr.sin_addr), htons(clientaddr.sin_port));

		char msg[50] = { "ID와 PW를 입력하세요(띄어쓰기로 구분)" };

		retval = send(client_sock, msg, strlen(msg), 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
			break;
		}

		int len;

		retval = recvn(client_sock, (char*)&len, sizeof(int), 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recv()");
			break;
		}
		else if (retval == 0)
			break;

		retval = recv(client_sock, buf, len, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recv()");
			break;
		}
		else if (retval == 0)
			break;

		buf[retval] = '\0';

		User *temp;
		temp = (User*)buf;

		if (strcmp(root.id, temp->id) == 0 && strcmp(root.pw, temp->pw) == 0)
		{
			printf("[TCP 서버] 클라이언트 %s:%d 로그인\n",
				inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			strcpy(msg, "로그인 성공");
		}
		else if (strcmp(root.id, temp->id) != 0)
		{
			strcpy(msg, "아이디가 다릅니다");
		}
		else if (strcmp(root.pw, temp->pw) != 0)
		{
			strcpy(msg, "비밀번호가 다릅니다");
		}

		retval = send(client_sock, msg, strlen(msg), 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
			break;
		}

		closesocket(client_sock);
	}
	closesocket(listen_sock);

	WSACleanup();
	return 0;
}