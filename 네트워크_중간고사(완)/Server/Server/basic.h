#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#include<WinSock2.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<time.h>

#define SERVERPORT 9000
#define BUFSIZE 4096
#define BREAK -1

//*********************************************************************************	�α��� �޽���
#define LOGIN_SUC "�α��� ����\n"
#define ID_ERROR "���̵� ��ġ���� �ʽ��ϴ�.\n"
#define PW_ERROR "�н����尡 ��ġ���� �ʽ��ϴ�.\n"
#define EXISTENT_ID "�̹� �����ϴ� ���̵��Դϴ�.\n"
#define ACC_CREATED "ȸ������ �Ǿ����ϴ�.\n"
//*********************************************************************************


//********************************************************************************* ���� �޽���
#define GAME_INTRO "������ �����մϴ�.\n1���� 9���� ������ ���ڸ� �Է��ϼ���\n"
#define GAME_OUTOFRANGE "1���� 9������ ���ڸ� �Է� �����մϴ�.\n"
#define GAME_OVERLAP "�ߺ��Ǵ� ���ڰ� �ֽ��ϴ�.\n"
//*********************************************************************************


//********************************************************************************* ȸ��Ż�� �޽���
#define DELETE_SUC "ȸ��Ż�� �Ǿ����ϴ�.\n"
#define DELETE_FAIL "ȸ�������� ��ġ���� �ʽ��ϴ�.\n"
//*********************************************************************************


//����, ���� ��ȯ ��������
enum RESULTS {
	SUCCESS = 1,	//����
	FAILED			//����
};

//���� ���� ��������
enum GAME {
	START = 1,		//����
	QUIT,			//����
	PLAY,			//�÷���
	CORRECT,		//����
};

//�޴� ��������
enum PROTOCOL {
	P_LOGIN = 1,	//�α���
	P_REGISTER,		//ȸ������
	P_DELETEACC,	//ȸ��Ż��
	P_RANK,			//��ũ����
	P_EXIT,			//����
};

//���� ����ü
typedef struct user
{
	char ID[255];	//���̵�
	char PW[255];	//�н�����
	int best_game;	//���� ���� ������ ���� Ƚ��
	int worst_game;	//���� �ʰ� ������ ���� Ƚ��
	int playtimes;	//�����÷��� Ƚ��
}_User;

// ����� ���� ������ ���� �Լ�
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
			return SOCKET_ERROR;
		}

		left -= received;
		ptr += received;
	}

	return (len - left);
}

// ���� �Լ� ���� ��� �� ����
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

// ���� �Լ� ���� ���
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