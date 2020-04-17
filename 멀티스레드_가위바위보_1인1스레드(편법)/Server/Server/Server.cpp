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

struct _ClientInfo {
	SOCKET sock;
	SOCKADDR_IN addr;
	char buf[BUFSIZE];
	int answer;			// 답
	int winflag = -1;	//	승리 판별
	int order;			// 차례
	int result;			// 결과 판별
	bool sendcheck = false;
	bool escapeflag = false;
	_ClientInfo* part;
};

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
bool MatchPartner(_ClientInfo* _ptr);
void WaitingPartner(_ClientInfo* _player);

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
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock,
			(SOCKADDR*)& clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			err_display("accept()");
			continue;
		}

		_ClientInfo* ptr = AddClientInfo(client_sock, clientaddr);

		if (MatchPartner(ptr))
		{
			ptr->part->order = 0;
			ptr->order = 1;

			HANDLE hThread0 = CreateThread(NULL, 0, ProcessClient, ptr, 0, NULL);
			HANDLE hThread1 = CreateThread(NULL, 0, ProcessClient, ptr->part, 0, NULL);
			if (hThread0 == NULL || hThread1 == NULL)
			{
				RemoveClientInfo(ptr);
				RemoveClientInfo(ptr->part);
				continue;
			}
			else
			{
				CloseHandle(hThread0);
				CloseHandle(hThread1);
			}
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
	_ClientInfo* player = (_ClientInfo*)arg;

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
		// 메시지, 결과 보내기
		size = PackPacket(player->buf, protocol, msg);

		retval = send(player->sock, player->buf, size, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
			break;
		}

		// 끝내기
		if (endflag)
			break;

		// 패킷 리시브
		if (!PacketRecv(player->sock, player->buf))
		{
			break;
		}

		WaitingPartner(player);

		// 프로토콜 얻기
		protocol = GetProtocol(player->buf);

		switch (protocol)
		{
		case PLAY:
			UnPackPacket(player->buf, &player->answer);
			printf("Player<%d> : %d\n", player->order, player->answer);
			break;
		}

		if (player->part != nullptr)
		{
			player->result = (3 + player->answer - player->part->answer) % 3;

			if (player->result == 0)		//	비김
				player->part->result = 0;
			else if (player->result == 1)	// 플레이어0 승리
				player->part->result = 2;
			else if (player->result == 2)	// 플레이어1 승리
				player->part->result = 1;
		}

		WaitingPartner(player);

		endflag = true;

		if (player->result == 0)	// 비김
		{
			protocol = PLAY;
			strcpy(msg, GAME_DRAW_MSG);
			endflag = false;
		}
		else if (player->result == 1)	// 승리
		{
			printf("플레이어%d 승리\n", player->order);

			protocol = RESULT;
			player->winflag = WIN;
			strcpy(msg, GAME_WIN_MSG);
		}
		else if (player->result == 2)	// 패배
		{
			printf("플레이어%d 패배\n", player->order);

			protocol = RESULT;
			player->winflag = LOSE;
			strcpy(msg, GAME_LOSE_MSG);
		}
	}

	RemoveClientInfo(player);

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

bool MatchPartner(_ClientInfo* _ptr)
{
	EnterCriticalSection(&cs);

	for (int i = 0; i < Count; i++)
	{
		if (_ptr != ClientInfo[i] && ClientInfo[i]->part == nullptr)
		{
			ClientInfo[i]->part = _ptr;
			_ptr->part = ClientInfo[i];
			LeaveCriticalSection(&cs);
			return true;
		}
	}
	LeaveCriticalSection(&cs);

	return false;
}

void WaitingPartner(_ClientInfo* _player)
{
	_player->sendcheck = true;

	while (true)
	{
		Sleep(100);
		if (_player->sendcheck && _player->part->sendcheck)
		{
			_player->part->escapeflag = true;
			break;
		}
		else if (_player->escapeflag)
			break;
	}

	_player->sendcheck = false;
	_player->escapeflag = false;
}