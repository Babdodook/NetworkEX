#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment (lib, "ws2_32.lib")
#include<WinSock2.h>
#include<stdio.h>
#include<stdlib.h>

#define SERVERPORT 9000
#define BUFSIZE 4096

#define INTRO_MSG "가위 바위 보 게임을 시작합니다.\n"
#define GAME_PLAY_MSG "1. 가위 2. 바위 3. 보\n"
#define GAME_DRAW_MSG "비겼습니다\n"
#define GAME_WIN_MSG "이겼습니다\n"
#define GAME_LOSE_MSG "졌습니다\n"

CRITICAL_SECTION cs;

enum PROTOCOL
{
	INTRO = 1,
	PLAY,
	RESULT
};

enum GAMECODE
{
	LOSE = 1,
	WIN = 2
};

typedef struct clinetinfo {
	SOCKET sock;
	SOCKADDR_IN addr;
	char buf[BUFSIZE];
	int answer;
	int winflag = -1;
}_ClientInfo;

typedef struct gamePlayers {
	_ClientInfo* player[2];
}_gamePlayer;


_ClientInfo* ClientInfo[100];
int Count = 0;

// About Client
_ClientInfo* AddClientInfo(SOCKET _sock, SOCKADDR_IN _addr);
void RemoveClientInfo(_ClientInfo* _ptr);

// Basics
SOCKET sock_init();
int recvn(SOCKET sock, char* buf, int len, int flags);
void err_quit(const char* msg);
void err_display(const char* msg);

// Pack
int PackPacket(char* _buf, PROTOCOL _protocol, const char* _str);

// UnPack
void UnPackPacket(const char* buf, int* answer);

// etc.
bool PacketRecv(SOCKET _sock, char* buf);
PROTOCOL GetProtocol(char* buf);

// Thread
DWORD WINAPI ProcessClient(LPVOID arg);

int main()
{
	InitializeCriticalSection(&cs);

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa)
		!= 0)
	{
		err_quit("WSAStartUP()");
	}

	SOCKET listen_sock = sock_init();

	SOCKADDR_IN clientaddr;
	SOCKET client_sock;
	int addrlen;

	HANDLE hThread;			// 스레드 핸들
	DWORD ThreadID;			// 스레드 아이디

	while (true)
	{
		_gamePlayer* gamePlayer;
		ZeroMemory(&gamePlayer, sizeof(gamePlayer));

		gamePlayer = new _gamePlayer();

		for (int i = 0; i < 2; i++)
		{
			addrlen = sizeof(clientaddr);
			client_sock = accept(listen_sock,
				(SOCKADDR*)& clientaddr, &addrlen);
			if (client_sock == INVALID_SOCKET)
			{
				err_display("accept()");
				continue;
			}

			_ClientInfo* ptr = AddClientInfo(client_sock, clientaddr);
			gamePlayer->player[i] = ptr;
		}

		hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)gamePlayer, 0, &ThreadID);

		if (hThread == NULL)
		{
			for (int i = 0; i < 2; i++)
				RemoveClientInfo(gamePlayer->player[i]);
			continue;
		}
		else
		{
			CloseHandle(hThread);
		}
	}

	closesocket(listen_sock);

	DeleteCriticalSection(&cs);

	WSACleanup();
	return 0;
}

_ClientInfo* AddClientInfo(SOCKET _sock, SOCKADDR_IN _addr)
{
	EnterCriticalSection(&cs);

	_ClientInfo* ptr = new _ClientInfo;
	ZeroMemory(ptr, sizeof(_ClientInfo));

	ptr->sock = _sock;
	memcpy(&ptr->addr, &_addr, sizeof(SOCKADDR_IN));

	ClientInfo[Count++] = ptr;

	LeaveCriticalSection(&cs);

	printf("클라이언트 접속:%s :%d\n",
		inet_ntoa(ptr->addr.sin_addr),
		ntohs(ptr->addr.sin_port));

	return ptr;
}

void RemoveClientInfo(_ClientInfo* _ptr)
{
	closesocket(_ptr->sock);

	printf("클라이언트 종료:%s :%d\n",
		inet_ntoa(_ptr->addr.sin_addr),
		ntohs(_ptr->addr.sin_port));

	EnterCriticalSection(&cs);

	for (int i = 0; i < Count; i++)
	{
		if (ClientInfo[i] == _ptr)
		{
			delete ClientInfo[i];

			for (int j = i; j < Count - 1; j++)
			{
				ClientInfo[j] = ClientInfo[j + 1];
			}
			ClientInfo[Count - 1] = nullptr;
			break;
		}
	}

	Count--;
	LeaveCriticalSection(&cs);
}

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
			break;
		}

		left -= received;
		ptr += received;
	}

	return (len - left);
}

void err_quit(const char* msg)
{
	LPVOID lpmsgbuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)& lpmsgbuf, 0, NULL);

	MessageBox(NULL, (LPCSTR)lpmsgbuf,
		msg, MB_ICONERROR);
	LocalFree(lpmsgbuf);
	exit(-1);
}

void err_display(const char* msg)
{
	LPVOID lpmsgbuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)& lpmsgbuf, 0, NULL);

	printf("[%s]%s\n", msg, (LPCSTR)lpmsgbuf);
	LocalFree(lpmsgbuf);
}

SOCKET sock_init()
{
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
	{
		err_quit("socket()");
	}

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SERVERPORT);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int retval = bind(listen_sock,
		(SOCKADDR*)& serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		err_quit("bind()");
	}

	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
	{
		err_quit("listen()");
	}

	return listen_sock;
}

bool PacketRecv(SOCKET _sock, char* buf)
{
	int size;

	int retval = recvn(_sock, (char*)& size,
		sizeof(int), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recv()");
		return false;
	}
	else if (retval == 0)
	{
		return false;
	}

	retval = recvn(_sock, buf, size, 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recv()");
		return false;
	}
	else if (retval == 0)
	{
		return false;
	}

	return true;
}

PROTOCOL GetProtocol(char* buf)
{
	PROTOCOL protocol;
	char* ptr = buf;
	memcpy(&protocol, ptr, sizeof(PROTOCOL));

	return protocol;
}

DWORD WINAPI ProcessClient(LPVOID arg)
{
	_gamePlayer* GP = (_gamePlayer*)arg;

	int size;
	int retval;
	int result;
	PROTOCOL protocol = INTRO;

	char msg[BUFSIZE];
	ZeroMemory(msg, BUFSIZE);
	strcpy(msg, INTRO_MSG);	

	bool endflag = false;
	while (true)
	{
		for (int i = 0; i < 2; i++)
		{
			if (GP->player[i]->winflag == WIN)
				strcpy(msg, GAME_WIN_MSG);
			else if(GP->player[i]->winflag == LOSE)
				strcpy(msg, GAME_LOSE_MSG);

			size = PackPacket(GP->player[i]->buf, protocol, msg);

			retval = send(GP->player[i]->sock, GP->player[i]->buf, size, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				break;
			}
		}

		if (endflag)
			break;

		for (int i = 0; i < 2; i++)
		{
			if (!PacketRecv(GP->player[i]->sock, GP->player[i]->buf))
			{
				break;
			}

			protocol = GetProtocol(GP->player[i]->buf);

			switch (protocol)
			{
			case PLAY:
				UnPackPacket(GP->player[i]->buf, &GP->player[i]->answer);
				printf("Player<%d> : %d\n", i, GP->player[i]->answer);
				break;
			}
		}

		result = (3 + GP->player[0]->answer - GP->player[1]->answer) % 3;
		
		if (result == 0)	// 비김
		{
			printf("비김\n");
			protocol = PLAY;
			strcpy(msg, GAME_DRAW_MSG);
		}
		else if (result == 1)	// player0 승리
		{
			printf("player0 승리\n");
			protocol = RESULT;
			GP->player[0]->winflag = WIN;
			GP->player[1]->winflag = LOSE;

			endflag = true;
		}
		else if (result == 2)	// player1 승리
		{
			printf("player1 승리\n");
			protocol = RESULT;
			GP->player[1]->winflag = WIN;
			GP->player[0]->winflag = LOSE;

			endflag = true;
		}
	}

	for (int i = 0; i < 2; i++)
		RemoveClientInfo(GP->player[i]);

	return 0;
}

int PackPacket(char* _buf, PROTOCOL _protocol, const char* _str)
{
	int size = 0;
	char* ptr = _buf + sizeof(int);
	int strsize = strlen(_str);

	memcpy(ptr, &_protocol, sizeof(PROTOCOL));
	size = size + sizeof(PROTOCOL);
	ptr = ptr + sizeof(PROTOCOL);

	memcpy(ptr, &strsize, sizeof(int));
	size = size + sizeof(int);
	ptr = ptr + sizeof(int);

	memcpy(ptr, _str, strsize);
	size = size + strsize;

	ptr = _buf;
	memcpy(ptr, &size, sizeof(int));

	return size + sizeof(int);
}

void UnPackPacket(const char* buf, int *answer)
{
	const char* ptr = buf + sizeof(PROTOCOL);

	memcpy(answer, ptr, sizeof(int));
	ptr = ptr + sizeof(int);
}