#pragma once
#include"basic.h"
#include"LinkedList.h"
#include"Packet.h"

//�α���
int Login(SOCKET client_sock, char* ptr, _User *User, _UserList *list)
{
	char msg[BUFSIZE];

	UnPacking(ptr, User);	//���� ���� ���̵�� �н����� ����ŷ�Ͽ� User�� ����

	Node *p = list->head;
	bool login_flag = false;

	RESULTS result = RESULTS::FAILED;
	strcpy(msg, ID_ERROR);
	while (p != nullptr)
	{
		//ID�� WP ��� ��ġ
		if (strcmp(p->data->ID, User->ID) == 0 && strcmp(p->data->PW, User->PW) == 0)
		{
			strcpy(msg, LOGIN_SUC);
			result = RESULTS::SUCCESS;
			login_flag = true;
			User = p->data;
			printf("�������� ���� - ID:%s PW:%s B:%d W:%d P:%d\n", User->ID, User->PW, User->best_game, User->worst_game, User->playtimes);
		}//���̵� ����ġ
		else if (strcmp(p->data->ID, User->ID) != 0 && strcmp(p->data->PW, User->PW) == 0)
		{
			strcpy(msg, ID_ERROR);
			result = RESULTS::FAILED;
		}//���̵� ��ġ �н����� ����ġ
		else if (strcmp(p->data->ID, User->ID) == 0 && strcmp(p->data->PW, User->PW) != 0)
		{
			strcpy(msg, PW_ERROR);
			result = RESULTS::FAILED;
		}
		p = p->link;
	}

	int size = 0;
	char buf[BUFSIZE];

	//��� �޽��� ��ŷ
	Packing(result, buf, &size, msg);
	//�α��� �޽��� ����
	int retval = send(client_sock, buf, size + sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send_1()");
		return BREAK;
	}

	return result;
}

//ȸ������
int Register(SOCKET client_sock, char* ptr, _User *User, _UserList *list)
{
	char msg[BUFSIZE];

	
	UnPacking(ptr, User);	//���� ���� ���̵�� �н����� ����ŷ�Ͽ� User�� ����

	Node *p = list->head;

	bool reg_flag = false;
	RESULTS result = RESULTS::FAILED;

	while (p != nullptr)
	{
		if (!strcmp(p->data->ID, User->ID))	//�̹� �����ϴ� ���̵��� ���
		{
			strcpy(msg, EXISTENT_ID);		//�޽��� ����
			reg_flag = true;				//�÷��� Ʈ��
			break;
		}
		p = p->link;
	}

	if (!reg_flag)	//��ġ�� ���̵� ���� ���
	{
		strcpy(msg, ACC_CREATED);			//���ԵǾ��ٴ� �޽��� ����
		Insert(list, User);					//����Ʈ�� �μ�Ʈ
		result = RESULTS::SUCCESS;			//���� ��������
	}

	int size = 0;
	char buf[BUFSIZE];

	Packing(result, buf, &size, msg);		//�޽����� �������� ��ŷ

	int retval = send(client_sock, buf, size + sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send_1()");
		return BREAK;
	}

	return result;
}

//����
int Game(SOCKET client_sock, char* ptr, _User *User, _UserList *list)
{
	int playtimes = 0;	//���� ���� Ƚ��
	int bestgame = 0;	//���� ���� ���� Ƚ��
	int worstgame = 0;	//���� �ʰ� ���� Ƚ��
	int strike = 0;		//��Ʈ����ũ
	int ball = 0;		//��
	int out = 0;		//�ƿ�

	while (1)
	{
		int size = 0;
		char buf[BUFSIZE];
		char msg[BUFSIZE];
		int correct_num[3];
		int answer[3];
		GAME result;
		strcpy(msg, GAME_INTRO);

		//������ ���� �ް�
		int retval = recvn(client_sock, (char*)&size, sizeof(int), 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recvn_size()");
			return BREAK;
		}
		else if (retval == 0)
		{
			return BREAK;
		}

		//���� �����ŭ ���۸� �о�´�
		retval = recvn(client_sock, buf, size, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recvn_buf()");
			return BREAK;
		}
		else if (retval == 0)
		{
			return BREAK;
		}

		//�������� ���� �д´�
		char *ptr = buf;
		memcpy(&result, ptr, sizeof(GAME));

		
		if (result == QUIT)	//������ ���
		{
			sprintf(msg, "���� ������ ���� ������ ���� �õ� Ƚ��: %d\n���� �ʰ� ���� ������ ���� �õ� Ƚ��: %d\n",
				bestgame, worstgame);	//�޽��� ����� ���� �����Ѵ�
			Packing(QUIT, buf, &size, msg);	//�޽����� ���������� ��ŷ�Ѵ�

			retval = send(client_sock, buf, size + sizeof(int), 0);	//Ŭ���̾�Ʈ���� ����
			if (retval == SOCKET_ERROR)
			{
				err_display("sned()");
				return BREAK;
			}

			//�����ϴ� �ܰ迡�� �ش� ������ ������ ��������� �Ѵ�.
			Node* p = list->head;
			while (p != nullptr)
			{
				if (strcmp(p->data->ID, User->ID) == 0 && strcmp(p->data->PW, User->PW) == 0)	//�ش��ϴ� ������ ã�Ƽ�
				{
					if(p->data->best_game > bestgame || p->data->best_game == 0)	//�̶����� ������ ���� Ƚ������ ���� ������ ���� �����
						p->data->best_game = bestgame;								//�� ������ �����Ѵ�.
					if(p->data->worst_game < worstgame || p->data->worst_game == 0)	//�ݴ�� �� �������� ���Ѱɷ� �����Ѵ�.
						p->data->worst_game = worstgame;

					p->data->playtimes += playtimes;	//�÷��� Ƚ���� �߰����ش�.

					break;
				}
				p = p->link;
			}

			FILE* fp = fopen("List.txt", "w+");		//������ ������ �����ؾ� �ϱ⶧���� ������ ����
			editList(fp, list);			//�غ��س��� ����Ʈ����Ʈ �Լ��� �������ش�.
			fclose(fp);					//������ �ݾ���

			return QUIT;
		}
		else if (result == START)		//���� ������ ���
		{
			result = PLAY;	//�������� �÷���
			int correct_count = 1;
			strike = 0;
			ball = 0;
			out = 0;

			//���� ����
			for (int i = 0; i < 3; i++)
			{
				correct_num[i] = rand() % 9 + 1;
				for (int j = 0; j < i; j++)
				{
					if (correct_num[i] == correct_num[j] && i != j)	//��ġ�� ������ ������, ��ġ�� �ٽ� ����
						i--;
				}
			}

			printf("������ ����:");		//����â�� ���� ���
			for (int i = 0; i < 3; i++)
				printf(" %d", correct_num[i]);
			printf("\n");

			while (1)	//���� �÷���
			{
				//�������ݰ� �޽����� ��ŷ, ���⼱ �������ݷ� ������ ����� ����(������ �������, ���������)
				Packing(result, buf, &size, msg);
				retval = send(client_sock, buf, size + sizeof(int), 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("send()");
					return BREAK;
				}

				if (result == CORRECT)
					break;

				retval = recvn(client_sock, (char*)&size, sizeof(int), 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("recvn()");
					return BREAK;
				}
				else if (retval == 0)
					return BREAK;

				retval = recvn(client_sock, buf, size, 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("recvn()");
					return BREAK;
				}
				else if (retval == 0)
					return BREAK;

				ptr = buf;
				memcpy(&result, ptr, sizeof(int));
				ptr += sizeof(int);

				//Ŭ�� ���� �޽��� ����ŷ, ������ �Է��� ������ �޾ƿ´�
				UnPacking(ptr, answer);

				printf("���� ����: %d %d %d\n", answer[0], answer[1], answer[2]);

				bool overlap = false;
				bool outofrange = false;

				//���� üũ
				for (int i = 0; i < 2; i++)
				{
					if (answer[i] == answer[i + 1])			// �ߺ��Ǵ� ���
						overlap = true;
				}

				for (int i = 0; i < 3; i++)
				{
					if (answer[i] <= 0 || 9 < answer[i])	// 1~9������ ���ڰ� �ƴ� ���
						outofrange = true;
				}

				if (!overlap && !outofrange)	//������ ���ٸ� ���� �˻��Ѵ�
				{
					//���� üũ
					for (int i = 0; i < 3; i++)
					{
						if (correct_num[i] == answer[i])	//�ڸ��� ���ڰ� ��ġ�ϸ� ��Ʈ����ũ
						{
							strike++;
						}

						for (int j = 0; j < 3; j++)			//���ڴ� �´µ� �ڸ��� �ٸ��� ��
						{
							if (i != j && correct_num[i] == answer[j])
							{
								ball++;
							}
						}
					}

					out = 3 - (strike + ball);		//�Ѵ� Ʋ�� ��� �ƿ��̴�.
					if (out < 0)					//�ƿ��� ��Ʈ����ũ�� ���� ��ģ�Ϳ� 3�� ���� �������� ��Ÿ���� �ȴ�
						out *= -1;					//(������ 3���̱� ������)
				}

				if (strike == 3)	//�� ���� ���
				{
					result = CORRECT;	//����ٴ� �������� �־���
					playtimes++;

					if (bestgame > correct_count || bestgame == 0)		//���� ���� Ƚ��, �ʰ� ���� Ƚ���� ��Ÿ���� ����
						bestgame = correct_count;
					if (worstgame < correct_count || worstgame == 0)
						worstgame = correct_count;

					sprintf(msg, "�����Դϴ�! �õ� Ƚ�� : %d��\n", correct_count);
				}
				else if (overlap)		//�ߺ��Ǵ� ���
				{
					strcpy(msg, GAME_OVERLAP);
				}
				else if (outofrange)	//������ ��� ���
				{
					strcpy(msg, GAME_OUTOFRANGE);
				}
				else
				{
					//��� �޽��� ����
					sprintf(msg, "%d Strike %d Ball %d Out\n", strike, ball, out);
					strike = 0;
					ball = 0;
					out = 0;
					correct_count++;
				}
			}
		}

	}
}

//ȸ�� Ż��
int DeleteAcc(SOCKET client_sock, char* ptr, _User *User, _UserList *list)
{
	char msg[BUFSIZE];
	RESULTS result;

	UnPacking(ptr, User);	//���� ���� ���̵�� �н����� ����ŷ�Ͽ� User�� ����

	Node* p = list->head;
	Node* prev = nullptr;
	bool flag = false;

	while (p != nullptr)
	{
		//ID�� WP ��� ��ġ�ϴ� ���(�ش� ���� ã�� ���)
		if (strcmp(p->data->ID, User->ID) == 0 && strcmp(p->data->PW, User->PW) == 0)
		{
			strcpy(msg, DELETE_SUC);
			result = RESULTS::SUCCESS;
			Delete(list, p, prev);	//���� ���ش�.
			flag = true;

			//���� ����
			if (list->head == nullptr)			//���� ��ϵ� ������ �Ѹ��ε� �����Ͽ��ٸ� head�� nullptr�� ���̴�.
			{									//�׷� ��쿡�� ������ �����ع�����.
				char filename[] = "List.txt";
				if (remove(filename) == 0)		//���� ã�� ���
					printf("���� ���� �Ϸ�\n");
				else
					printf("���� ���� ����\n");
			}
			else								//�Ѹ��� �ƴ� ���, �������� �ʰ� ������ �Ѵ�.
			{
				FILE* fp = fopen("List.txt", "w+");	//���� ����
				editList(fp, list);					//�����ϰ�
				fclose(fp);							//�ݴ´�
			}

			break;
		}
		prev = p;
		p = p->link;
	}

	if (!flag)	//�ش� ���� ��ã�� ��
	{
		strcpy(msg, DELETE_FAIL);	//���� ���� �޽��� ����
	}

	int size = 0;
	char buf[BUFSIZE];
	
	//���������� ���� �������. �޽����� �� ���޵Ǹ� �ȴ�.
	Packing(result, buf, &size, msg);
	//ȸ��Ż�� ��� �޽��� ����
	int retval = send(client_sock, buf, size + sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send_1()");
		return BREAK;
	}
}

//��ũ ����
int RankInfo(SOCKET client_sock, _UserList *list)
{
	int size = 0;
	int retval;
	char buf[BUFSIZE];
	char temp[BUFSIZE];
	char msg[BUFSIZE] = " ";

	Node *p = list->head;

	if (p != nullptr)
	{
		while (p != nullptr)
		{
			//���̵�, ����Ʈ����, ����Ʈ����, �÷���Ÿ��
			sprintf(msg, "ID: %s  [Best Game: %d  Worst Game: %d  Playtime: %d]\n",
				p->data->ID, p->data->best_game, p->data->worst_game, p->data->playtimes);

			//�������ݰ� �޽��� ��ŷ
			Packing(P_RANK, buf, &size, msg);

			//������ ����
			retval = send(client_sock, buf, size + sizeof(int), 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				return BREAK;
			}

			p = p->link;
		}
	}

	//�� ���� ��� ������ �������ݰ� ���� �޽��� ��ŷ
	Packing(P_EXIT, buf, &size, msg);

	//����
	retval = send(client_sock, buf, size + sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send()");
		return BREAK;
	}
}