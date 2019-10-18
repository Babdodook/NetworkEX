#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment (lib, "ws2_32.lib")
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define SERVERPORT 9000
#define BUFSIZE 4096

#define INTRO_MSG "1���� 100������ ���ڸ� �Է��ϼ���.\n"
#define UP_MSG "UP��\n"
#define DOWN_MSG "DOWN��\n"
#define CLEAR_MSG "�����Դϴ�!\n"

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

//�޽��� ��ŷ
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

//���� ����ŷ
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

		printf("Ŭ���̾�Ʈ ����:%s :%d\n",
			inet_ntoa(clientaddr.sin_addr),
			ntohs(clientaddr.sin_port));

		int randnum = rand() % 100 + 1;
		PROTOCOL protocol = INTRO;
		strcpy(msg, INTRO_MSG);
		
		printf("������� : %d\n", randnum);

		while (1)
		{
			//��ŷ
			int size = PackPacket(buf, protocol, msg);

			retval = send(client_sock, buf, size, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				break;
			}

			int answer = 0;

			//��Ŷ �ޱ�
			if (!PacketRecv(client_sock, buf))
			{
				break;
			}

			//�������� �����
			protocol = GetProtocol(buf);

			//����ŷ
			answer = UnPackPacket(buf);

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

		printf("Ŭ���̾�Ʈ ����:%s :%d\n",
			inet_ntoa(clientaddr.sin_addr),
			ntohs(clientaddr.sin_port));
	}

	closesocket(listen_sock);

	WSACleanup();
	return 0;
}