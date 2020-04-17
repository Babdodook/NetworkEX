#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment (lib, "ws2_32.lib")
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define SERVERPORT 9000
#define BUFSIZE 4096

#define INTRO_MSG "1부터 100사이의 숫자를 입력하세요.\n"
#define UP_MSG "UP↑\n"
#define DOWN_MSG "DOWN↓\n"
#define CLEAR_MSG "정답입니다!\n"

enum PROTOCOL
{
	INTRO = 1,
	GAMEPLAY,
	CLEAR
};


int recvn(SOCKET sock, char* buf, int len, int flags)
{
	int received;
	char* ptr = buf;
	int left = len;

	while (left > 0)
	{
		received = recv(sock, ptr, left, flags);
		if (received == SOCKET_ERROR)
		{
			return SOCKET_ERROR;
		}
		else if (received == 0)
		{
			break;
		}

		left -= received;
		ptr += received;
	}

	return (len - left);
}

void err_quit(const char* msg)
{
	LPVOID lpmsgbuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&lpmsgbuf, 0, NULL);

	MessageBox(NULL, (LPCSTR)lpmsgbuf,
		msg, MB_ICONERROR);
	LocalFree(lpmsgbuf);
	exit(-1);
}

void err_display(const char* msg)
{
	LPVOID lpmsgbuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&lpmsgbuf, 0, NULL);

	printf("[%s]%s\n", msg, (LPCSTR)lpmsgbuf);
	LocalFree(lpmsgbuf);
}

int main()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa)
		!= 0)
	{
		err_quit("WSAStartUP()");
	}

	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
	{
		err_quit("socket()");
	}

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SERVERPORT);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int retval = bind(listen_sock,
		(SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		err_quit("bind()");
	}

	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
	{
		err_quit("listen()");
	}

	SOCKADDR_IN clientaddr;
	int addrlen;
	SOCKET client_sock;
	char buf[BUFSIZE];
	char msg[BUFSIZE];

	while (1)
	{
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock,
			(SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			err_display("accept()");
			continue;
		}

		printf("클라이언트 접속:%s :%d\n",
			inet_ntoa(clientaddr.sin_addr),
			ntohs(clientaddr.sin_port));

		int randnum = rand() % 100 + 1;
		PROTOCOL protocol = INTRO;
		strcpy(msg, INTRO_MSG);
		
		while (1)
		{
			int strsize = strlen(msg);

			int size = 0;
			char* ptr = buf + sizeof(int);
			
			memcpy(ptr, &protocol, sizeof(PROTOCOL));
			size = size + sizeof(PROTOCOL);
			ptr = ptr + sizeof(PROTOCOL);

			memcpy(ptr, &strsize, sizeof(int));
			size = size + sizeof(int);
			ptr = ptr + sizeof(int);

			memcpy(ptr, msg, strsize);
			size = size + strsize;

			ptr = buf;
			memcpy(ptr, &size, sizeof(int));

			retval = send(client_sock, buf, size + sizeof(int), 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				break;
			}

			int answer;

			retval = recvn(client_sock, (char*)&answer,
				sizeof(int), 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("recv()");
				break;
			}
			else if (retval == 0)
			{
				break;
			}

			protocol = GAMEPLAY;
			if (answer == randnum)
			{
				protocol = CLEAR;
				strcpy(msg, CLEAR_MSG);
			}
			else if (answer < randnum)
			{
				strcpy(msg, UP_MSG);
			}
			else
			{
				strcpy(msg, DOWN_MSG);
			}
		}

		closesocket(client_sock);

		printf("클라이언트 종료:%s :%d\n",
			inet_ntoa(clientaddr.sin_addr),
			ntohs(clientaddr.sin_port));
	}

	closesocket(listen_sock);

	WSACleanup();
	return 0;
}