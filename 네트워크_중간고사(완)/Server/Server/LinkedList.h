#pragma once
#include"basic.h"

//노드 구조체
typedef struct Node {
	_User *data;
	Node *link;
}Node;

//헤드와 테일을 가진 유저리스트 구조체
typedef struct _UserList {
	Node *head;
	Node *tale;
}_UserList;

//리스트 초기화 함수
void List_Init(_UserList* Userlist)
{
	Userlist->head = (Node*)malloc(sizeof(Node));
	Userlist->head = nullptr;
	Userlist->tale = (Node*)malloc(sizeof(Node));
	Userlist->tale = nullptr;
}

void loadList(FILE* fp, _UserList *list);
void saveList(FILE* fp, Node *tale);

//추가
void Insert(_UserList* list, _User *user)
{
	//새로운 노드 생성과 초기화
	Node* new_node = (Node*)malloc(sizeof(Node));
	new_node->data = (_User*)malloc(sizeof(_User));
	strcpy(new_node->data->ID, user->ID);
	strcpy(new_node->data->PW, user->PW);
	new_node->data->best_game = 0;
	new_node->data->worst_game = 0;
	new_node->data->playtimes = 0;
	new_node->link = nullptr;

	if (list->head == nullptr)			//리스트가 비어있을 때
	{
		list->head = new_node;			//헤드에 노드 넣고
		list->head->link = nullptr;		//링크는 nullptr
		list->tale = list->head;		//테일도 헤드랑 같도록
	}
	else if (list->head != nullptr)		//비어있지 않을 때
	{
		list->tale->link = new_node;	//꼬리의 다음에 새로운 노드 추가
		list->tale = new_node;			//그리고 꼬리는 새로운 노드의 위치로
		list->tale->link = nullptr;		//꼬리의 링크는 nullptr
	}

	printf("[아이디 생성] ID:%s PW:%s\n", list->tale->data->ID, list->tale->data->PW);

	FILE* fp = fopen("List.txt", "at+");	//파일 열기

	if (fp)	//파일 열기 성공
	{
		saveList(fp, list->tale);			//새로운 노드 정보 추가
		fclose(fp);							//파일 닫기
		printf("파일 저장 완료\n");
	}
	else
		printf("파일 저장 실패\n");

}

//삭제
void Delete(_UserList* list, Node* p, Node* prev)
{
	if (list->head->link == nullptr)		//리스트에 노드가 하나뿐일 때
	{
		free(p);		//동적할당 해제
		p = nullptr;

		list->head = nullptr;
	}
	else if (list->head->link != nullptr && prev == nullptr)	//노드 두개일 경우 첫번째 노드 삭제 시
	{
		list->head = p->link;		//바로 앞 노드를 헤드에 넣고
		list->head->link = nullptr;	//헤드 다음은 nullptr
		list->tale = list->head;	//꼬리와 헤드의 위치가 같도록

		free(p);
		p = nullptr;
	}
	else if (list->head != nullptr)	//비어있지 않을 경우
	{
		prev->link = p->link;

		if (prev->link == nullptr)
			list->tale = prev;

		free(p);
		p = nullptr;
	}
}

//파일에 유저 정보 추가
void saveList(FILE* fp, Node *tale)
{
	fprintf(fp, "%s %s %d %d %d\n", tale->data->ID, tale->data->PW,
		tale->data->best_game, tale->data->worst_game, tale->data->playtimes);
}

//파일에 저장된 유저 정보 리스트로 불러오기
void loadList(FILE* fp, _UserList* list)
{
	if (fp != NULL)
	{
		while (feof(fp) == 0)	//파일의 끝을 만날때까지
		{
			Node* new_node = (Node*)malloc(sizeof(Node));					//노드 동적할당
			new_node->data = (_User*)malloc(sizeof(_User));					//데이터도 동적할당
			memset(&new_node->data->ID, 0, sizeof(new_node->data->ID));
			memset(&new_node->data->PW, 0, sizeof(new_node->data->PW));
			new_node->data->best_game = 0;
			new_node->data->worst_game = 0;
			new_node->data->playtimes = 0;
			new_node->link = nullptr;

			fscanf(fp, "%s %s %d %d %d\n", &new_node->data->ID, &new_node->data->PW,
				&new_node->data->best_game, &new_node->data->worst_game, &new_node->data->playtimes);

			//인서트 부분과 동일함
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

//파일 정보 수정해야 하는 경우
//파일에서 원하는 정보만 찾아서 수정하고 싶었지만,
//먼소리인지 알아먹을 수가 없어서 그냥 리스트 통으로 다시 저장했습니다
void editList(FILE *fp, _UserList *list)
{
	Node *p = list->head;

	if (p->link == nullptr)	//노드가 하나일 경우
	{
		//헤드정보만 저장하면 끝
		fprintf(fp, "%s %s %d %d %d\n", list->head->data->ID, list->head->data->PW,
			list->head->data->best_game, list->head->data->worst_game, list->head->data->playtimes);
	}
	else					//두 개 이상인 경우
	{
		while (p != nullptr)	//리스트 전부 저장
		{
			fprintf(fp, "%s %s %d %d %d\n", p->data->ID, p->data->PW,
				p->data->best_game, p->data->worst_game, p->data->playtimes);

			p = p->link;
		}
	}
}

//소멸자
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