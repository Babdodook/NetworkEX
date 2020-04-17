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
		int size;

		//ũ����� ����
		int retval = recvn(sock, (char*)&size, sizeof(int), 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recv()");
			break;
		}
		else if (retval == 0)
		{
			break;
		}

		//��ü������ ����� ũ�⸸ŭ�� �������� (�������� ����)
		retval = recvn(sock, buf, size, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recv()");
			break;
		}
		else if (retval == 0)
		{
			break;
		}

		PROTOCOL protocol;
		char msg[BUFSIZE];
		int strsize;
		bool flag = false;
		char* ptr = buf;
		int answer;

		//�������� ���� ����
		memcpy(&protocol, ptr, sizeof(PROTOCOL));
		ptr = ptr + sizeof(PROTOCOL);

		switch (protocol)
		{
			//�޾Ƽ� �޽����� ����ϸ� �Ǳ⶧���� ������ �Ȱ���
		case INTRO:
		case GAMEPLAY:
		case CLEAR:
			memcpy(&strsize, ptr, sizeof(int));
			ptr = ptr + sizeof(int);

			memcpy(msg, ptr, strsize);

			msg[strsize] = '\0';
			printf("%s\n", msg);
			
			//Ŭ�����϶��� �÷��� Ʈ��� ���Ϲ� Ż��
			if(protocol == CLEAR)
				flag = true;

			break;
		}

		if (flag)
		{
			break;
		}

		printf("�Է�: ");
		scanf("%d", &answer);

		retval = send(sock, (char*)&answer, sizeof(int), 0);
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