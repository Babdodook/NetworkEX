#pragma once
#include"basic.h"
#include"Packet.h"

//메뉴 선택
enum MENU{
	M_LOGIN=1,
	M_REG,
	M_DEL,
	M_RANK,
	M_EXIT
};

int MainMenu()	//메인 메뉴
{
	system("cls");
	int select;

	printf("<메뉴>\n");
	printf("1. 로그인\n");
	printf("2. 회원가입\n");
	printf("3. 회원탈퇴\n");
	printf("4. 랭크정보\n");
	printf("5. 종료\n");
	printf("선택: ");
	scanf("%d", &select);

	return select;
}

//로그인
int Login(SOCKET sock)
{
	system("cls");

	_User user;

	//아이디와 비밀번호 입력
	printf("ID: ");
	scanf("%s", user.ID);
	printf("PW: ");
	scanf("%s", user.PW);

	int size = 0;
	char buf[BUFSIZE];
	PROTOCOL protocol = P_LOGIN;	//로그인 프로토콜

	//프로토콜과 기입한 아이디 비밀번호 패킹
	Packing(protocol, buf, &size, user);

	//보냄
	int retval = send(sock, buf, size + sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send()");
		return BREAK;
	}

	//사이즈 먼저 받아서
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

	//사이즈만큼 버퍼에서 읽음
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

	//프로토콜 먼저 구분
	memcpy(&result, ptr, sizeof(RESULTS));
	ptr += sizeof(RESULTS);

	char msg[BUFSIZE];
	//메시지 언패킹
	UnPacking(ptr, msg);

	printf("%s", msg);

	system("pause");
	if (result == SUCCESS)	//로그인 성공한 경우
		return SUCCESS;
}

int Register(SOCKET sock)
{
	system("cls");

	_User user;

	//아이디와 비밀번호 입력
	printf("ID: ");
	scanf("%s", user.ID);
	printf("PW: ");
	scanf("%s", user.PW);

	int size = 0;
	char buf[BUFSIZE];
	PROTOCOL protocol = P_REGISTER;	//회원가입 프로토콜

	//프로토콜과 아이디, 비번을 패킹
	Packing(protocol, buf, &size, user);

	//보냄
	int retval = send(sock, buf, size + sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send()");
		return BREAK;
	}

	//사이즈 먼저 받아서
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

	//사이즈만큼 버퍼에서 읽는다
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

	//프로토콜 먼저 분리
	memcpy(&result, ptr, sizeof(RESULTS));
	ptr += sizeof(RESULTS);

	char msg[BUFSIZE];
	//메시지 언패킹
	UnPacking(ptr, msg);

	printf("%s", msg);

	system("pause");
}

//게임
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

		printf("<숫자야구 게임>\n");
		printf("1. 게임시작\n");
		printf("2. 종료\n");
		printf("선택: ");
		scanf("%d", &select);

		//프로토콜을 설정
		if (select == START)
			result = START;
		else if (select == QUIT)
			result = QUIT;

		strcpy(msg, " ");
		//프로토콜을 패킹하여
		Packing(result, buf, &size, msg);

		//보낸다
		int retval = send(sock, buf, size + sizeof(int), 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
			return BREAK;
		}

		//게임시작
		if (result == START)
		{
			system("cls");

			bool endgame = false;
			while (1)
			{
				//사이즈를 받아
				retval = recvn(sock, (char*)&size, sizeof(int), 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("recvn()");
					return BREAK;
				}
				else if (retval == 0)
					return BREAK;

				//사이즈 만큼 버퍼에서 읽는다
				retval = recvn(sock, buf, size, 0);
				if (retval == SOCKET_ERROR)
				{
					err_display("recvn()");
					return BREAK;
				}
				else if (retval == 0)
					return BREAK;

				ptr = buf;
				//프로토콜을 읽고
				memcpy(&result, ptr, sizeof(GAME));
				ptr += sizeof(GAME);

				//메시지를 언패킹한다
				UnPacking(ptr, msg);
				printf("%s", msg);

				switch (result)	//프로토콜에 따라 정답인지 아닌지 판별한다
				{
				case PLAY:		//정답이 아님, 계속해서 플레이
					printf("입력<띄어쓰기 구분>: ");
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
				case CORRECT:	//정답 게임 끝
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
		else if (result == QUIT)	//종료
		{
			//메시지 받음
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
			//프로토콜 분리
			memcpy(&result, ptr, sizeof(GAME));
			ptr += sizeof(GAME);

			//메시지 언패킹
			UnPacking(ptr, msg);
			printf("%s", msg);

			system("pause");
			return QUIT;
		}
	}
}

//회원탈퇴
int DeleteAcc(SOCKET sock)
{
	system("cls");

	_User user;

	//아이디와 비밀번호 입력
	printf("ID: ");
	scanf("%s", user.ID);
	printf("PW: ");
	scanf("%s", user.PW);

	int size = 0;
	char buf[BUFSIZE];
	PROTOCOL protocol = P_DELETEACC;	//회원탈퇴 프로토콜

	Packing(protocol, buf, &size, user);	//프로토콜과 아이디, 비번 패킹

	//서버에게 보낸다
	int retval = send(sock, buf, size + sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send()");
		return BREAK;
	}

	//데이터를 받는다
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

	//프로토콜을 먼저 읽는다
	memcpy(&result, ptr, sizeof(RESULTS));
	ptr += sizeof(RESULTS);

	//메시지를 언패킹
	UnPacking(ptr, msg);
	printf("%s", msg);

	system("pause");
}

//랭크 정보
int RankInfo(SOCKET sock)
{
	system("cls");

	char buf[BUFSIZE];
	char msg[BUFSIZE];
	int size = 0;
	PROTOCOL protocol = P_RANK;	//랭크정보 프로토콜

	strcpy(msg, " ");
	Packing(protocol, buf, &size, msg);	//프로토콜을 패킹

	//서버에게 보냄
	int retval = send(sock, buf, size + sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send()");
		return BREAK;
	}

	printf("[User Information]\n");
	while (1)
	{
		//데이터를 받음
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

		//프로토콜을 읽고
		memcpy(&result, ptr, sizeof(PROTOCOL));
		ptr += sizeof(PROTOCOL);

		if (result == P_EXIT)	//종료인경우 탈출
			break;

		UnPacking(ptr, msg);	//메시지 언패킹하여 출력
		printf("%s", msg);

	}

	system("pause");
}