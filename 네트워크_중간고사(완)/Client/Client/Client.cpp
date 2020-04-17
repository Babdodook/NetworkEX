#pragma once
#include"basic.h"
#include"menu.h"

int main()
{
	int retval;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");

	char buf[BUFSIZE];
	bool endflag = false;

	//������ ������ ���
	while (1)
	{
		int result;
		//���θ޴� ���
		switch (MainMenu())
		{
		case M_LOGIN:	//�α���
			result = Login(sock);
			if (result == BREAK)
				endflag = true;
			else if (result == SUCCESS)	//�α��� �����̸� ���� ����
			{
				if (Game(sock) == BREAK)
					endflag = true;
			}
			break;
		case M_REG:		//ȸ������
			if (Register(sock) == BREAK)
				endflag = true;
			break;
		case M_DEL:		//ȸ��Ż��
			if (DeleteAcc(sock) == BREAK)
				endflag = true;
			break;
		case M_RANK:	//��ũ����
			if (RankInfo(sock) == BREAK)
				endflag = true;
			break;
		case M_EXIT:	//����
			endflag = true;
			break;
		}

		//�����÷��� true�� ����
		if (endflag)
			break;
	}

	// closesocket()
	closesocket(sock);

	// ���� ����
	WSACleanup();
	return 0;
}