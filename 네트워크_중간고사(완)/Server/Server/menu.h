#pragma once
#include"basic.h"
#include"LinkedList.h"
#include"Packet.h"

//로그인
int Login(SOCKET client_sock, char* ptr, _User *User, _UserList *list)
{
	char msg[BUFSIZE];

	UnPacking(ptr, User);	//받은 유저 아이디와 패스워드 언패킹하여 User에 저장

	Node *p = list->head;
	bool login_flag = false;

	RESULTS result = RESULTS::FAILED;
	strcpy(msg, ID_ERROR);
	while (p != nullptr)
	{
		//ID와 WP 모두 일치
		if (strcmp(p->data->ID, User->ID) == 0 && strcmp(p->data->PW, User->PW) == 0)
		{
			strcpy(msg, LOGIN_SUC);
			result = RESULTS::SUCCESS;
			login_flag = true;
			User = p->data;
			printf("현재유저 정보 - ID:%s PW:%s B:%d W:%d P:%d\n", User->ID, User->PW, User->best_game, User->worst_game, User->playtimes);
		}//아이디 불일치
		else if (strcmp(p->data->ID, User->ID) != 0 && strcmp(p->data->PW, User->PW) == 0)
		{
			strcpy(msg, ID_ERROR);
			result = RESULTS::FAILED;
		}//아이디 일치 패스워드 불일치
		else if (strcmp(p->data->ID, User->ID) == 0 && strcmp(p->data->PW, User->PW) != 0)
		{
			strcpy(msg, PW_ERROR);
			result = RESULTS::FAILED;
		}
		p = p->link;
	}

	int size = 0;
	char buf[BUFSIZE];

	//결과 메시지 패킹
	Packing(result, buf, &size, msg);
	//로그인 메시지 보냄
	int retval = send(client_sock, buf, size + sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send_1()");
		return BREAK;
	}

	return result;
}

//회원가입
int Register(SOCKET client_sock, char* ptr, _User *User, _UserList *list)
{
	char msg[BUFSIZE];

	
	UnPacking(ptr, User);	//받은 유저 아이디와 패스워드 언패킹하여 User에 저장

	Node *p = list->head;

	bool reg_flag = false;
	RESULTS result = RESULTS::FAILED;

	while (p != nullptr)
	{
		if (!strcmp(p->data->ID, User->ID))	//이미 존재하는 아이디일 경우
		{
			strcpy(msg, EXISTENT_ID);		//메시지 복사
			reg_flag = true;				//플래그 트루
			break;
		}
		p = p->link;
	}

	if (!reg_flag)	//겹치는 아이디가 없는 경우
	{
		strcpy(msg, ACC_CREATED);			//가입되었다는 메시지 복사
		Insert(list, User);					//리스트에 인서트
		result = RESULTS::SUCCESS;			//성공 프로토콜
	}

	int size = 0;
	char buf[BUFSIZE];

	Packing(result, buf, &size, msg);		//메시지와 프로토콜 패킹

	int retval = send(client_sock, buf, size + sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send_1()");
		return BREAK;
	}

	return result;
}

//게임
int Game(SOCKET client_sock, char* ptr, _User *User, _UserList *list)
{
	int playtimes = 0;	//게임 진행 횟수
	int bestgame = 0;	//가장 빨리 맞춘 횟수
	int worstgame = 0;	//가장 늦게 맞춘 횟수
	int strike = 0;		//스트라이크
	int ball = 0;		//볼
	int out = 0;		//아웃

	while (1)
	{
		int size = 0;
		char buf[BUFSIZE];
		char msg[BUFSIZE];
		int correct_num[3];
		int answer[3];
		GAME result;
		strcpy(msg, GAME_INTRO);

		//사이즈 먼저 받고
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

		//받은 사이즈만큼 버퍼를 읽어온다
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

		//프로토콜 먼저 읽는다
		char *ptr = buf;
		memcpy(&result, ptr, sizeof(GAME));

		
		if (result == QUIT)	//끝내는 경우
		{
			sprintf(msg, "가장 빠르게 맞춘 게임의 정답 시도 횟수: %d\n가장 늦게 맞춘 게임의 정답 시도 횟수: %d\n",
				bestgame, worstgame);	//메시지 출력을 위해 복사한다
			Packing(QUIT, buf, &size, msg);	//메시지와 프로토콜을 패킹한다

			retval = send(client_sock, buf, size + sizeof(int), 0);	//클라이언트에게 전송
			if (retval == SOCKET_ERROR)
			{
				err_display("sned()");
				return BREAK;
			}

			//종료하는 단계에서 해당 유저의 정보를 수정해줘야 한다.
			Node* p = list->head;
			while (p != nullptr)
			{
				if (strcmp(p->data->ID, User->ID) == 0 && strcmp(p->data->PW, User->PW) == 0)	//해당하는 유저를 찾아서
				{
					if(p->data->best_game > bestgame || p->data->best_game == 0)	//이때까지 유저가 맞춘 횟수보다 현재 진행한 게임 결과가
						p->data->best_game = bestgame;								//더 좋으면 수정한다.
					if(p->data->worst_game < worstgame || p->data->worst_game == 0)	//반대로 더 못했으면 못한걸로 수정한다.
						p->data->worst_game = worstgame;

					p->data->playtimes += playtimes;	//플레이 횟수를 추가해준다.

					break;
				}
				p = p->link;
			}

			FILE* fp = fopen("List.txt", "w+");		//파일의 내용을 수정해야 하기때문에 파일을 연다
			editList(fp, list);			//준비해놓은 에디트리스트 함수로 수정해준다.
			fclose(fp);					//파일을 닫아줌

			return QUIT;
		}
		else if (result == START)		//게임 진행인 경우
		{
			result = PLAY;	//프로토콜 플레이
			int correct_count = 1;
			strike = 0;
			ball = 0;
			out = 0;

			//난수 생성
			for (int i = 0; i < 3; i++)
			{
				correct_num[i] = rand() % 9 + 1;
				for (int j = 0; j < i; j++)
				{
					if (correct_num[i] == correct_num[j] && i != j)	//겹치는 정답이 없도록, 겹치면 다시 생성
						i--;
				}
			}

			printf("생성된 정답:");		//서버창에 정답 출력
			for (int i = 0; i < 3; i++)
				printf(" %d", correct_num[i]);
			printf("\n");

			while (1)	//게임 플레이
			{
				//프로토콜과 메시지를 패킹, 여기선 프로토콜로 게임의 결과를 구분(정답을 맞췄는지, 못맞췄는지)
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

				//클라가 보낸 메시지 언패킹, 유저가 입력한 정답을 받아온다
				UnPacking(ptr, answer);

				printf("받은 정답: %d %d %d\n", answer[0], answer[1], answer[2]);

				bool overlap = false;
				bool outofrange = false;

				//에러 체크
				for (int i = 0; i < 2; i++)
				{
					if (answer[i] == answer[i + 1])			// 중복되는 경우
						overlap = true;
				}

				for (int i = 0; i < 3; i++)
				{
					if (answer[i] <= 0 || 9 < answer[i])	// 1~9사이의 숫자가 아닌 경우
						outofrange = true;
				}

				if (!overlap && !outofrange)	//에러가 없다면 답을 검사한다
				{
					//정답 체크
					for (int i = 0; i < 3; i++)
					{
						if (correct_num[i] == answer[i])	//자리와 숫자가 일치하면 스트라이크
						{
							strike++;
						}

						for (int j = 0; j < 3; j++)			//숫자는 맞는데 자리가 다르면 볼
						{
							if (i != j && correct_num[i] == answer[j])
							{
								ball++;
							}
						}
					}

					out = 3 - (strike + ball);		//둘다 틀릴 경우 아웃이다.
					if (out < 0)					//아웃은 스트라이크와 볼을 합친것에 3을 빼고 절댓값으로 나타내면 된다
						out *= -1;					//(정답은 3개이기 때문에)
				}

				if (strike == 3)	//다 맞춘 경우
				{
					result = CORRECT;	//맞췄다는 프로토콜 넣어줌
					playtimes++;

					if (bestgame > correct_count || bestgame == 0)		//빨리 맞춘 횟수, 늦게 맞춘 횟수를 나타내기 위함
						bestgame = correct_count;
					if (worstgame < correct_count || worstgame == 0)
						worstgame = correct_count;

					sprintf(msg, "정답입니다! 시도 횟수 : %d번\n", correct_count);
				}
				else if (overlap)		//중복되는 경우
				{
					strcpy(msg, GAME_OVERLAP);
				}
				else if (outofrange)	//범위를 벗어난 경우
				{
					strcpy(msg, GAME_OUTOFRANGE);
				}
				else
				{
					//결과 메시지 복사
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

//회원 탈퇴
int DeleteAcc(SOCKET client_sock, char* ptr, _User *User, _UserList *list)
{
	char msg[BUFSIZE];
	RESULTS result;

	UnPacking(ptr, User);	//받은 유저 아이디와 패스워드 언패킹하여 User에 저장

	Node* p = list->head;
	Node* prev = nullptr;
	bool flag = false;

	while (p != nullptr)
	{
		//ID와 WP 모두 일치하는 경우(해당 유저 찾은 경우)
		if (strcmp(p->data->ID, User->ID) == 0 && strcmp(p->data->PW, User->PW) == 0)
		{
			strcpy(msg, DELETE_SUC);
			result = RESULTS::SUCCESS;
			Delete(list, p, prev);	//삭제 해준다.
			flag = true;

			//파일 수정
			if (list->head == nullptr)			//만약 등록된 유저가 한명인데 삭제하였다면 head가 nullptr일 것이다.
			{									//그럴 경우에는 파일을 삭제해버린다.
				char filename[] = "List.txt";
				if (remove(filename) == 0)		//파일 찾은 경우
					printf("파일 삭제 완료\n");
				else
					printf("파일 삭제 실패\n");
			}
			else								//한명이 아닌 경우, 삭제하지 않고 수정만 한다.
			{
				FILE* fp = fopen("List.txt", "w+");	//파일 열고
				editList(fp, list);					//수정하고
				fclose(fp);							//닫는다
			}

			break;
		}
		prev = p;
		p = p->link;
	}

	if (!flag)	//해당 유저 못찾을 시
	{
		strcpy(msg, DELETE_FAIL);	//삭제 실패 메시지 복사
	}

	int size = 0;
	char buf[BUFSIZE];
	
	//프로토콜은 별로 상관없음. 메시지만 잘 전달되면 된다.
	Packing(result, buf, &size, msg);
	//회원탈퇴 결과 메시지 보냄
	int retval = send(client_sock, buf, size + sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send_1()");
		return BREAK;
	}
}

//랭크 정보
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
			//아이디, 베스트게임, 워스트게임, 플레이타임
			sprintf(msg, "ID: %s  [Best Game: %d  Worst Game: %d  Playtime: %d]\n",
				p->data->ID, p->data->best_game, p->data->worst_game, p->data->playtimes);

			//프로토콜과 메시지 패킹
			Packing(P_RANK, buf, &size, msg);

			//데이터 보냄
			retval = send(client_sock, buf, size + sizeof(int), 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				return BREAK;
			}

			p = p->link;
		}
	}

	//다 보낸 경우 끝내는 프로토콜과 공백 메시지 패킹
	Packing(P_EXIT, buf, &size, msg);

	//보냄
	retval = send(client_sock, buf, size + sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send()");
		return BREAK;
	}
}