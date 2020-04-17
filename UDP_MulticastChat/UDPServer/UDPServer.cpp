#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define ROOMLIST "����Ʈ\n1.�ڿ��ù�(225.0.0.1)\n2.����ȯ��(225.0.0.2)\n3.��õ����(225.0.0.3)\n"
#define ROOM_ONE "225.0.0.1"
#define ROOM_TWO "225.0.0.2"
#define ROOM_THREE "225.0.0.3"

enum SELECT_TYPE
{
	LIST = 0,
	ONE,
	TWO,
	THREE
};

#define SERVERPORT 9000
#define BUFSIZE    512

// ���� �Լ� ���� ��� �� ����
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

// ���� �Լ� ���� ���
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



int main(int argc, char *argv[])
{
	int retval;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("bind()");

	// ������ ��ſ� ����� ����
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE+1];

	// Ŭ���̾�Ʈ�� ������ ���

	while (1)
	{
		// ������ �ޱ�
		addrlen = sizeof(clientaddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(SOCKADDR*)&clientaddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
		}

		// ���� ������ ���
		buf[retval] = '\0';
		printf("[UDP/%s:%d] %s ����\n", inet_ntoa(clientaddr.sin_addr),
			ntohs(clientaddr.sin_port), buf);

		// ���� ��Ŷ�� ���� ����Ʈ, �� �ּ� �Ѱ��ֱ�
		switch (atoi(buf))
		{
		case SELECT_TYPE::LIST:
			strcpy(buf, ROOMLIST);
			break;
		case SELECT_TYPE::ONE:
			strcpy(buf, ROOM_ONE);
			break;
		case SELECT_TYPE::TWO:
			strcpy(buf, ROOM_TWO);
			break;
		case SELECT_TYPE::THREE:
			strcpy(buf, ROOM_THREE);
			break;
		}

		// ������ ������ - �������ּ� ������
		retval = sendto(sock, buf, strlen(buf), 0,
			(SOCKADDR*)&clientaddr, sizeof(clientaddr));
		if (retval == SOCKET_ERROR) {
			err_display("sendto()");
		}
		printf("���������� : %s\n", buf);
		printf("[UDP] %d����Ʈ�� ���½��ϴ�.\n", retval);
	}
	system("pause");
	// closesocket()
	closesocket(sock);

	// ���� ����
	WSACleanup();
	return 0;
}