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
	GAMEPLAY,
	CLEAR
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

//패킷 받기
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

//정답 패킹
int PackPacket(char *buf, PROTOCOL protocol, int data)
{
	int size = 0;
	char* ptr = buf + sizeof(int);

	memcpy(ptr, &protocol, sizeof(PROTOCOL));
	size += sizeof(PROTOCOL);
	ptr += sizeof(PROTOCOL);

	memcpy(ptr, &data, sizeof(int));
	size += sizeof(int);

	ptr = buf;
	memcpy(ptr, &size, sizeof(int));

	return size + sizeof(int);
}

//메시지 언패킹
void UnPackPacket(char* buf, char *msg)
{
	int strsize = 0;
	char* ptr = buf + sizeof(PROTOCOL);
	
	memcpy(&strsize, ptr, sizeof(int));
	ptr = ptr + sizeof(int);

	memcpy(msg, ptr, strsize);
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
		if (!PacketRecv(sock, buf))	//패킷 받기
		{
			break;
		}

		PROTOCOL protocol;
		char msg[BUFSIZE];
		ZeroMemory(msg, sizeof(msg));
		bool flag = false;
		int answer = 0;

		//프로토콜 먼저 읽음
		protocol = GetProtocol(buf);

		switch (protocol)
		{
			//받아서 메시지만 출력하면 되기때문에 형식이 똑같음
		case INTRO:
		case GAMEPLAY:
		case CLEAR:
			UnPackPacket(buf, msg);		//언패킹
			printf("%s\n", msg);
			
			//클리어일때는 플래그 트루로 와일문 탈출
			if(protocol == CLEAR)
				flag = true;

			break;
		}

		if (flag)
		{
			break;
		}

		printf("입력: ");
		scanf("%d", &answer);

		int size = PackPacket(buf, GAMEPLAY, answer);	//패킹

		retval = send(sock, buf, size, 0);
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