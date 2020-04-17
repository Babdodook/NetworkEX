#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment (lib, "ws2_32.lib")
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

#define SERVERIP "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 4096

enum PROTOCOL
{
	INTRO = 1,
	PLAY,
	RESULT
};

// Basics
SOCKET sock_init();
int recvn(SOCKET sock, char* buf, int len, int flags);
void err_quit(const char* msg);
void err_display(const char* msg);

// PackPacket
int PackPacket(char* _buf, PROTOCOL _protocol, int answer);

// UnPackPacket
void UnPackPacket(const char* buf, char* _str);

// etc.
bool PacketRecv(SOCKET _sock, char* buf);
PROTOCOL GetProtocol(char* buf);

int main()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa)
		!= 0)
	{
		err_quit("WSAStartUP()");
	}

	SOCKET sock = sock_init();

	char buf[BUFSIZE];
	ZeroMemory(buf, BUFSIZE);
	
	bool endflag = false;

	while (true)
	{
		if (!PacketRecv(sock, buf))
		{
			break;
		}

		PROTOCOL protocol = GetProtocol(buf);

		char msg[BUFSIZE];
		ZeroMemory(msg, BUFSIZE);
		switch (protocol)
		{
		case INTRO:
		case PLAY:
		case RESULT:
			UnPackPacket(buf, msg);
			printf("%s", msg);
			
			if (protocol == RESULT)
				endflag = true;

			break;
		}

		if (endflag)
			break;

		int answer;

		printf("\n1. 가위 2. 바위 3. 보\n");
		printf("입력>> ");
		scanf("%d", &answer);

		int size = PackPacket(buf, PLAY, answer);

		int retval = send(sock, buf, size, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
			break;
		}

		
	}

	return 0;
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
		(LPSTR)& lpmsgbuf, 0, NULL);

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
		(LPSTR)& lpmsgbuf, 0, NULL);

	printf("[%s]%s\n", msg, (LPCSTR)lpmsgbuf);
	LocalFree(lpmsgbuf);
}

SOCKET sock_init()
{
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		err_quit("socket()");
	}

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SERVERPORT);
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);

	int retval = connect(sock, (SOCKADDR*)& serveraddr,
		sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		err_quit("connect()");
	}

	return sock;
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

PROTOCOL GetProtocol(char* buf)
{
	PROTOCOL protocol;
	char* ptr = buf;
	memcpy(&protocol, ptr, sizeof(PROTOCOL));

	return protocol;
}

void UnPackPacket(const char* buf, char* _str)
{
	int strsize = strlen(_str);
	const char* ptr = buf + sizeof(PROTOCOL);

	memcpy(&strsize, ptr, sizeof(int));
	ptr = ptr + sizeof(int);

	memcpy(_str, ptr, strsize);
	ptr = ptr + strsize;
}

int PackPacket(char* _buf, PROTOCOL _protocol, int answer)
{
	int size = 0;
	char* ptr = _buf + sizeof(int);
	
	memcpy(ptr, &_protocol, sizeof(PROTOCOL));
	size = size + sizeof(PROTOCOL);
	ptr = ptr + sizeof(PROTOCOL);

	memcpy(ptr, &answer, sizeof(int));
	size = size + sizeof(int);

	ptr = _buf;
	memcpy(ptr, &size, sizeof(int));

	return size + sizeof(int);
}