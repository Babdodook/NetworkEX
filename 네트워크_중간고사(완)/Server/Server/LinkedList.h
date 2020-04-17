#pragma once
#include"basic.h"

//��� ����ü
typedef struct Node {
	_User *data;
	Node *link;
}Node;

//���� ������ ���� ��������Ʈ ����ü
typedef struct _UserList {
	Node *head;
	Node *tale;
}_UserList;

//����Ʈ �ʱ�ȭ �Լ�
void List_Init(_UserList* Userlist)
{
	Userlist->head = (Node*)malloc(sizeof(Node));
	Userlist->head = nullptr;
	Userlist->tale = (Node*)malloc(sizeof(Node));
	Userlist->tale = nullptr;
}

void loadList(FILE* fp, _UserList *list);
void saveList(FILE* fp, Node *tale);

//�߰�
void Insert(_UserList* list, _User *user)
{
	//���ο� ��� ������ �ʱ�ȭ
	Node* new_node = (Node*)malloc(sizeof(Node));
	new_node->data = (_User*)malloc(sizeof(_User));
	strcpy(new_node->data->ID, user->ID);
	strcpy(new_node->data->PW, user->PW);
	new_node->data->best_game = 0;
	new_node->data->worst_game = 0;
	new_node->data->playtimes = 0;
	new_node->link = nullptr;

	if (list->head == nullptr)			//����Ʈ�� ������� ��
	{
		list->head = new_node;			//��忡 ��� �ְ�
		list->head->link = nullptr;		//��ũ�� nullptr
		list->tale = list->head;		//���ϵ� ���� ������
	}
	else if (list->head != nullptr)		//������� ���� ��
	{
		list->tale->link = new_node;	//������ ������ ���ο� ��� �߰�
		list->tale = new_node;			//�׸��� ������ ���ο� ����� ��ġ��
		list->tale->link = nullptr;		//������ ��ũ�� nullptr
	}

	printf("[���̵� ����] ID:%s PW:%s\n", list->tale->data->ID, list->tale->data->PW);

	FILE* fp = fopen("List.txt", "at+");	//���� ����

	if (fp)	//���� ���� ����
	{
		saveList(fp, list->tale);			//���ο� ��� ���� �߰�
		fclose(fp);							//���� �ݱ�
		printf("���� ���� �Ϸ�\n");
	}
	else
		printf("���� ���� ����\n");

}

//����
void Delete(_UserList* list, Node* p, Node* prev)
{
	if (list->head->link == nullptr)		//����Ʈ�� ��尡 �ϳ����� ��
	{
		free(p);		//�����Ҵ� ����
		p = nullptr;

		list->head = nullptr;
	}
	else if (list->head->link != nullptr && prev == nullptr)	//��� �ΰ��� ��� ù��° ��� ���� ��
	{
		list->head = p->link;		//�ٷ� �� ��带 ��忡 �ְ�
		list->head->link = nullptr;	//��� ������ nullptr
		list->tale = list->head;	//������ ����� ��ġ�� ������

		free(p);
		p = nullptr;
	}
	else if (list->head != nullptr)	//������� ���� ���
	{
		prev->link = p->link;

		if (prev->link == nullptr)
			list->tale = prev;

		free(p);
		p = nullptr;
	}
}

//���Ͽ� ���� ���� �߰�
void saveList(FILE* fp, Node *tale)
{
	fprintf(fp, "%s %s %d %d %d\n", tale->data->ID, tale->data->PW,
		tale->data->best_game, tale->data->worst_game, tale->data->playtimes);
}

//���Ͽ� ����� ���� ���� ����Ʈ�� �ҷ�����
void loadList(FILE* fp, _UserList* list)
{
	if (fp != NULL)
	{
		while (feof(fp) == 0)	//������ ���� ����������
		{
			Node* new_node = (Node*)malloc(sizeof(Node));					//��� �����Ҵ�
			new_node->data = (_User*)malloc(sizeof(_User));					//�����͵� �����Ҵ�
			memset(&new_node->data->ID, 0, sizeof(new_node->data->ID));
			memset(&new_node->data->PW, 0, sizeof(new_node->data->PW));
			new_node->data->best_game = 0;
			new_node->data->worst_game = 0;
			new_node->data->playtimes = 0;
			new_node->link = nullptr;

			fscanf(fp, "%s %s %d %d %d\n", &new_node->data->ID, &new_node->data->PW,
				&new_node->data->best_game, &new_node->data->worst_game, &new_node->data->playtimes);

			//�μ�Ʈ �κа� ������
			if (list->head == nullptr)
			{
				list->head = new_node;
				list->head->link = nullptr;
				list->tale = list->head;
			}
			else if (list->head != nullptr)
			{
				list->tale->link = new_node;
				list->tale = new_node;
				list->tale->link = nullptr;
			}
		}
	}
}

//���� ���� �����ؾ� �ϴ� ���
//���Ͽ��� ���ϴ� ������ ã�Ƽ� �����ϰ� �;�����,
//�ռҸ����� �˾Ƹ��� ���� ��� �׳� ����Ʈ ������ �ٽ� �����߽��ϴ�
void editList(FILE *fp, _UserList *list)
{
	Node *p = list->head;

	if (p->link == nullptr)	//��尡 �ϳ��� ���
	{
		//��������� �����ϸ� ��
		fprintf(fp, "%s %s %d %d %d\n", list->head->data->ID, list->head->data->PW,
			list->head->data->best_game, list->head->data->worst_game, list->head->data->playtimes);
	}
	else					//�� �� �̻��� ���
	{
		while (p != nullptr)	//����Ʈ ���� ����
		{
			fprintf(fp, "%s %s %d %d %d\n", p->data->ID, p->data->PW,
				p->data->best_game, p->data->worst_game, p->data->playtimes);

			p = p->link;
		}
	}
}

//�Ҹ���
void freeNodes(Node* head)
{
	Node *last = head;
	Node * temp;

	while (last != nullptr)
	{
		temp = last->link;
		free(last);
		last = temp;
	}
}