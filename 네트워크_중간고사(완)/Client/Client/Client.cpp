#pragma once
#include"basic.h"
#include"menu.h"

int main()
{
	int retval;

	// 윈속 초기화
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

	//서버와 데이터 통신
	while (1)
	{
		int result;
		//메인메뉴 출력
		switch (MainMenu())
		{
		case M_LOGIN:	//로그인
			result = Login(sock);
			if (result == BREAK)
				endflag = true;
			else if (result == SUCCESS)	//로그인 성공이면 게임 시작
			{
				if (Game(sock) == BREAK)
					endflag = true;
			}
			break;
		case M_REG:		//회원가입
			if (Register(sock) == BREAK)
				endflag = true;
			break;
		case M_DEL:		//회원탈퇴
			if (DeleteAcc(sock) == BREAK)
				endflag = true;
			break;
		case M_RANK:	//랭크정보
			if (RankInfo(sock) == BREAK)
				endflag = true;
			break;
		case M_EXIT:	//종료
			endflag = true;
			break;
		}

		//엔드플래그 true면 종료
		if (endflag)
			break;
	}

	// closesocket()
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}