#pragma comment (lib, "ws2_32.lib")
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

#define SERVERIP "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 4096

enum RESULT
{
	ID_ERROR = 1,
	PW_ERROR,
	LOGIN
};

enum PROTOCOL
{
	INTRO = 1,
	LOGIN_INFO,
	LOGIN_RESULT
};

struct _LoginInfo
{
	char ID[255];
	char PW[255];
};


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

bool PacketRecv(SOCKET _sock, char *buf)
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

int PackPacket(char *buf, PROTOCOL _protocol, char *str1, char *str2)
{
	int size = 0;
	int strsize = strlen(str1);
	char *ptr = buf + sizeof(int);

	memcpy(ptr, &_protocol, sizeof(PROTOCOL));
	ptr = ptr + sizeof(PROTOCOL);
	size = size + sizeof(PROTOCOL);

	memcpy(ptr, &strsize, sizeof(int));
	ptr += sizeof(int);
	size += sizeof(int);

	memcpy(ptr, str1, strsize);
	ptr += strsize;
	size += strsize;

	strsize = strlen(str2);
	memcpy(ptr, &strsize, sizeof(int));
	ptr += sizeof(int);
	size += sizeof(int);

	memcpy(ptr, str2, strsize);
	size += strsize;

	ptr = buf;
	memcpy(ptr, &size, sizeof(int));

	return size + sizeof(int);
}

PROTOCOL GetProtocol(char *buf)
{
	PROTOCOL protocol;
	char *ptr = buf;
	memcpy(&protocol, ptr, sizeof(PROTOCOL));

	return protocol;
}

void UnPackPacket(const char *buf, char *_str)
{
	int strsize = strlen(_str);
	const char *ptr = buf + sizeof(PROTOCOL);

	memcpy(&strsize, ptr, sizeof(int));
	ptr = ptr + sizeof(int);

	memcpy(_str, ptr, strsize);
	ptr = ptr + strsize;
}

void UnPackPacket(const char *buf, int _result, char *_str)
{
	int strsize = strlen(_str);
	const char *ptr = buf + sizeof(PROTOCOL);

	memcpy(&_result, ptr, sizeof(int));
	ptr = ptr + sizeof(int);

	memcpy(&strsize, ptr, sizeof(int));
	ptr = ptr + sizeof(int);

	memcpy(_str, ptr, strsize);
	ptr = ptr + strsize;
}

int main()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa)
		!= 0)
	{
		err_quit("WSAStartUP()");
	}

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

	int retval = connect(sock, (SOCKADDR*)&serveraddr,
					sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		err_quit("connect()");
	}

	char buf[BUFSIZE];

	while (1)
	{
		if (!PacketRecv(sock, buf))
		{
			break;
		}

		char msg[BUFSIZE];
		ZeroMemory(&msg, sizeof(msg));
		RESULT result;
		ZeroMemory(&result, sizeof(RESULT));
		int strsize;
		bool flag = false;
		char* ptr = buf;

		PROTOCOL protocol = GetProtocol(buf);

		switch (protocol)
		{
		case INTRO:
			UnPackPacket(buf, msg);
			printf("%s\n", msg);
			break;

		case LOGIN_RESULT:
			UnPackPacket(buf, result, msg);
			printf("%s\n", msg);

			if (result == LOGIN)
			{
				flag = true;
			}
			else
			{
				continue;
			}

			break;

		}	

		if (flag)
		{
			break;
		}
	
		_LoginInfo logininfo;
		ZeroMemory(&logininfo, sizeof(_LoginInfo));
		
		printf("ID:");
		scanf("%s", logininfo.ID);
		printf("PW:");
		scanf("%s", logininfo.PW);

		int size = PackPacket(buf, LOGIN_INFO, logininfo.ID, logininfo.PW);

		retval = send(sock, buf, size,0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
			break;
		}

	}

	system("pause");
	closesocket(sock);

	WSACleanup();
	return 0;
}