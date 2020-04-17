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

//*********************************************************************************	로그인 메시지
#define LOGIN_SUC "로그인 성공\n"
#define ID_ERROR "아이디가 일치하지 않습니다.\n"
#define PW_ERROR "패스워드가 일치하지 않습니다.\n"
#define EXISTENT_ID "이미 존재하는 아이디입니다.\n"
#define ACC_CREATED "회원가입 되었습니다.\n"
//*********************************************************************************


//********************************************************************************* 게임 메시지
#define GAME_INTRO "게임을 시작합니다.\n1에서 9까지 세가지 숫자를 입력하세요\n"
#define GAME_OUTOFRANGE "1에서 9까지의 숫자만 입력 가능합니다.\n"
#define GAME_OVERLAP "중복되는 숫자가 있습니다.\n"
//*********************************************************************************


//********************************************************************************* 회원탈퇴 메시지
#define DELETE_SUC "회원탈퇴 되었습니다.\n"
#define DELETE_FAIL "회원정보가 일치하지 않습니다.\n"
//*********************************************************************************


//성공, 실패 반환 프로토콜
enum RESULTS {
	SUCCESS = 1,	//성공
	FAILED			//실패
};

//게임 진행 프로토콜
enum GAME {
	START = 1,		//시작
	QUIT,			//종료
	PLAY,			//플레이
	CORRECT,		//정답
};

//메뉴 프로토콜
enum PROTOCOL {
	P_LOGIN = 1,	//로그인
	P_REGISTER,		//회원가입
	P_DELETEACC,	//회원탈퇴
	P_RANK,			//랭크정보
	P_EXIT,			//종료
};

//유저 구조체
typedef struct user
{
	char ID[255];	//아이디
	char PW[255];	//패스워드
	int best_game;	//가장 빨리 정답을 맞춘 횟수
	int worst_game;	//가장 늦게 정답을 맞춘 횟수
	int playtimes;	//게임플레이 횟수
}_User;

// 사용자 정의 데이터 수신 함수
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

// 소켓 함수 오류 출력 후 종료
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

// 소켓 함수 오류 출력
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