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

typedef struct clinetinfo {
	SOCKET sock;
	SOCKADDR_IN addr;
	char buf[BUFSIZE];
}_ClientInfo;

_ClientInfo* ClientInfo[100];
int Count = 0;

_ClientInfo* AddClientInfo(SOCKET _sock, SOCKADDR_IN _addr)
{
	_ClientInfo* ptr = new _ClientInfo;
	ZeroMemory(ptr, sizeof(_ClientInfo));

	ptr->sock = _sock;
	memcpy(&ptr->addr, &_addr, sizeof(SOCKADDR_IN));

	printf("클라이언트 접속:%s :%d\n",
		inet_ntoa(ptr->addr.sin_addr),
		ntohs(ptr->addr.sin_port));

	ClientInfo[Count++] = ptr;
	return ptr;
}

void RemoveClientInfo(_ClientInfo* _ptr)
{
	closesocket(_ptr->sock);

	printf("클라이언트 종료:%s :%d\n",
		inet_ntoa(_ptr->addr.sin_addr),
		ntohs(_ptr->addr.sin_port));

	for (int i = 0; i < Count; i++)
	{
		if (ClientInfo[i] == _ptr)
		{
			delete ClientInfo[i];

			for (int j = i; j < Count - 1; j++)
			{
				ClientInfo[j] = ClientInfo[j + 1];
			}
			ClientInfo[Count - 1] = nullptr;
			break;
		}
	}

	Count--;
}

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

bool PacketRecv(SOCKET _sock, char* buf)
{
	int size;

	int retval = recvn(_sock, (char*)&size,
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

//메시지 패킹
int PackPacket(char *buf, PROTOCOL protocol, char *msg)
{
	int size = 0;
	int strsize = strlen(msg);
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

	return size + sizeof(int);
}

//정답 언패킹
int UnPackPacket(const char *buf)
{
	int data;
	const char* ptr = buf + sizeof(PROTOCOL);

	memcpy(&data, ptr, sizeof(int));

	return data;
}

PROTOCOL GetProtocol(char* buf)
{
	PROTOCOL protocol;
	char* ptr = buf;
	memcpy(&protocol, ptr, sizeof(PROTOCOL));

	return protocol;
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

		_ClientInfo* ptr = AddClientInfo(client_sock, clientaddr);

		int randnum = rand() % 100 + 1;
		PROTOCOL protocol = INTRO;
		strcpy(msg, INTRO_MSG);
		
		printf("정답생성 : %d\n", randnum);

		while (1)
		{
			//패킹
			int size = PackPacket(ptr->buf, protocol, msg);

			retval = send(ptr->sock, ptr->buf, size, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				break;
			}

			int answer = 0;

			//패킷 받기
			if (!PacketRecv(ptr->sock, ptr->buf))
			{
				break;
			}

			//프로토콜 떼어내기
			protocol = GetProtocol(ptr->buf);

			//언패킹
			answer = UnPackPacket(ptr->buf);

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

		RemoveClientInfo(ptr);
	}

	closesocket(listen_sock);

	WSACleanup();
	return 0;
}