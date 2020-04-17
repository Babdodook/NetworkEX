#pragma once
#include"basic.h"
#include"Packet.h"

//�޴� ����
enum MENU{
	M_LOGIN=1,
	M_REG,
	M_DEL,
	M_RANK,
	M_EXIT
};

int MainMenu()	//���� �޴�
{
	system("cls");
	int select;

	printf("<�޴�>\n");
	printf("1. �α���\n");
	printf("2. ȸ������\n");
	printf("3. ȸ��Ż��\n");
	printf("4. ��ũ����\n");
	printf("5. ����\n");
	printf("����: ");
	scanf("%d", &select);

	return select;
}

//�α���
int Login(SOCKET sock)
{
	system("cls");

	_User user;

	//���̵�� ��й�ȣ �Է�
	printf("ID: ");
	scanf("%s", user.ID);
	printf("PW: ");
	scanf("%s", user.PW);

	int size = 0;
	char buf[BUFSIZE];
	PROTOCOL protocol = P_LOGIN;	//�α��� ��������

	//�������ݰ� ������ ���̵� ��й�ȣ ��ŷ
	Packing(protocol, buf, &size, user);

	//����
	int retval = send(sock, buf, size + sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send()");
		return BREAK;
	}

	//������ ���� �޾Ƽ�
	retval = recvn(sock, (char*)&size, sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recvn_size()");
		return BREAK;
	}
	else if (retval == 0)
	{
		return BREAK;
	}

	//�����ŭ ���ۿ��� ����
	retval = recvn(sock, buf, size, 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recvn_buf()");
		return BREAK;
	}
	else if (retval == 0)
	{
		return BREAK;
	}

	char* ptr = buf;
	RESULTS result;

	//�������� ���� ����
	memcpy(&result, ptr, sizeof(RESULTS));
	ptr += sizeof(RESULTS);

	char msg[BUFSIZE];
	//�޽��� ����ŷ
	UnPacking(ptr, msg);

	printf("%s", msg);

	system("pause");
	if (result == SUCCESS)	//�α��� ������ ���
		return SUCCESS;
}

int Register(SOCKET sock)
{
	system("cls");

	_User user;

	//���̵�� ��й�ȣ �Է�
	printf("ID: ");
	scanf("%s", user.ID);
	printf("PW: ");
	scanf("%s", user.PW);

	int size = 0;
	char buf[BUFSIZE];
	PROTOCOL protocol = P_REGISTER;	//ȸ������ ��������

	//�������ݰ� ���̵�, ����� ��ŷ
	Packing(protocol, buf, &size, user);

	//����
	int retval = send(sock, buf, size + sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send()");
		return BREAK;
	}

	//������ ���� �޾Ƽ�
	retval = recvn(sock, (char*)&size, sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recvn_size()");
		return BREAK;
	}
	else if (retval == 0)
	{
		return BREAK;
	}

	//�����ŭ ���ۿ��� �д´�
	retval = recvn(sock, buf, size, 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recvn_buf()");
		return BREAK;
	}
	else if (retval == 0)
	{
		return BREAK;
	}

	char* ptr = buf;
	RESULTS result;

	//�������� ���� �и�
	memcpy(&result, ptr, sizeof(RESULTS));
	ptr += sizeof(RESULTS);

	char msg[BUFSIZE];
	//�޽��� ����ŷ
	UnPacking(ptr, msg);

	printf("%s", msg);

	system("pause");
}

//����
int Game(SOCKET sock)
{
	while (1)
	{
		system("cls");

		int select;
		int size = 0;
		char buf[BUFSIZE];
		char msg[BUFSIZE];
		int answer[3];
		GAME result;
		char* ptr;
		bool endflag = false;

		printf("<���ھ߱� ����>\n");
		printf("1. ���ӽ���\n");
		printf("2. ����\n");
		printf("����: ");
		scanf("%d", &select);

		//���������� ����
		if (select == START)
			result = START;
		else if (select == QUIT)
			result = QUIT;

		strcpy(msg, " ");
		//���������� ��ŷ�Ͽ�
		Packing(result, buf, &size, msg);

		//������
		int retval = send(sock, buf, size + sizeof(int), 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
			return BREAK;
		}

		//���ӽ���
		if (result == START)
		{
			system("cls");

			bool endgame = false;
			while (1)
			{
				//����� �޾�
				retval = recvn(sock, (char*)&size, sizeof(int), 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("recvn()");
					return BREAK;
				}
				else if (retval == 0)
					return BREAK;

				//������ ��ŭ ���ۿ��� �д´�
				retval = recvn(sock, buf, size, 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("recvn()");
					return BREAK;
				}
				else if (retval == 0)
					return BREAK;

				ptr = buf;
				//���������� �а�
				memcpy(&result, ptr, sizeof(GAME));
				ptr += sizeof(GAME);

				//�޽����� ����ŷ�Ѵ�
				UnPacking(ptr, msg);
				printf("%s", msg);

				switch (result)	//�������ݿ� ���� �������� �ƴ��� �Ǻ��Ѵ�
				{
				case PLAY:		//������ �ƴ�, ����ؼ� �÷���
					printf("�Է�<���� ����>: ");
					for (int i = 0; i < 3; i++)
						scanf("%d", &answer[i]);

					Packing(result, buf, &size, answer);
					retval = send(sock, buf, size + sizeof(int), 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("send()");
						return BREAK;
					}

					break;
				case CORRECT:	//���� ���� ��
					endgame = true;
					break;
				}

				if (endgame)
				{
					system("pause");
					break;
				}
			}
		}
		else if (result == QUIT)	//����
		{
			//�޽��� ����
			retval = recvn(sock, (char*)&size, sizeof(int), 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("recvn()");
				return BREAK;
			}
			else if (retval == 0)
				return BREAK;

			retval = recvn(sock, buf, size, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("recvn()");
				return BREAK;
			}
			else if (retval == 0)
				return BREAK;

			ptr = buf;
			//�������� �и�
			memcpy(&result, ptr, sizeof(GAME));
			ptr += sizeof(GAME);

			//�޽��� ����ŷ
			UnPacking(ptr, msg);
			printf("%s", msg);

			system("pause");
			return QUIT;
		}
	}
}

//ȸ��Ż��
int DeleteAcc(SOCKET sock)
{
	system("cls");

	_User user;

	//���̵�� ��й�ȣ �Է�
	printf("ID: ");
	scanf("%s", user.ID);
	printf("PW: ");
	scanf("%s", user.PW);

	int size = 0;
	char buf[BUFSIZE];
	PROTOCOL protocol = P_DELETEACC;	//ȸ��Ż�� ��������

	Packing(protocol, buf, &size, user);	//�������ݰ� ���̵�, ��� ��ŷ

	//�������� ������
	int retval = send(sock, buf, size + sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send()");
		return BREAK;
	}

	//�����͸� �޴´�
	retval = recvn(sock, (char*)&size, sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recvn()");
		return BREAK;
	}
	else if (retval == 0)
		return BREAK;

	retval = recvn(sock, buf, size, 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recvn()");
		return BREAK;
	}
	else if (retval == 0)
		return BREAK;

	char msg[BUFSIZE];
	char *ptr = buf;
	RESULTS result;

	//���������� ���� �д´�
	memcpy(&result, ptr, sizeof(RESULTS));
	ptr += sizeof(RESULTS);

	//�޽����� ����ŷ
	UnPacking(ptr, msg);
	printf("%s", msg);

	system("pause");
}

//��ũ ����
int RankInfo(SOCKET sock)
{
	system("cls");

	char buf[BUFSIZE];
	char msg[BUFSIZE];
	int size = 0;
	PROTOCOL protocol = P_RANK;	//��ũ���� ��������

	strcpy(msg, " ");
	Packing(protocol, buf, &size, msg);	//���������� ��ŷ

	//�������� ����
	int retval = send(sock, buf, size + sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send()");
		return BREAK;
	}

	printf("[User Information]\n");
	while (1)
	{
		//�����͸� ����
		retval = recvn(sock, (char*)&size, sizeof(int), 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recvn()");
			return BREAK;
		}
		else if (retval == 0)
			return BREAK;

		retval = recvn(sock, buf, size, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recvn()");
			return BREAK;
		}
		else if (retval == 0)
			return BREAK;

		char *ptr = buf;
		PROTOCOL result;

		//���������� �а�
		memcpy(&result, ptr, sizeof(PROTOCOL));
		ptr += sizeof(PROTOCOL);

		if (result == P_EXIT)	//�����ΰ�� Ż��
			break;

		UnPacking(ptr, msg);	//�޽��� ����ŷ�Ͽ� ���
		printf("%s", msg);

	}

	system("pause");
}