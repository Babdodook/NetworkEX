#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERPORT 9000
#define BUFSIZE    4096

#define PLAYER_COUNT 100

#define INTRO_MSG "가위(0), 바위(1), 보(2)중 하나를 입력하세요:"
#define WIN_MSG "이겼습니다.\n"
#define LOSE_MSG "졌습니다.\n"
#define NO_WIN_MSG "비겼습니다\n"

CRITICAL_SECTION cs;

enum RESULT
{	
	NODATA=-1,
	WIN = 1,
	LOSE,
	NO_WIN
};

enum PROTOCOL
{
	INTRO=1,
	GAME_VALUE,
	GAME_RESULT
};

struct _ClientInfo
{
	SOCKET sock;
	SOCKADDR_IN addr;
	char buf[BUFSIZE];
	int game_value;
	_ClientInfo* part;
	bool game_value_flag;
	bool no_win_flag;
	bool end_flag;
	HANDLE hWaitPart;
};

_ClientInfo* ClientInfo[PLAYER_COUNT];
int Count = 0;

void err_quit(const char *msg);
void err_display(const char *msg);

int recvn(SOCKET s, char *buf, int len, int flags);

int who_win(int first, int second);

bool PacketRecv(SOCKET _sock, char* _buf);
PROTOCOL GetProtocol(const char* _ptr);
int PackPacket(char* _buf, PROTOCOL _protocol, const char* _str1);
int PackPacket(char* _buf, PROTOCOL _protocol, RESULT _result, const char* _str1);

void UnPackPacket(const char* _buf, int& _data);

_ClientInfo* AddClientInfo(SOCKET _sock, SOCKADDR_IN _addr);
void RemoveClientInfo(_ClientInfo* _ptr);

bool MatchPartner(_ClientInfo*);
DWORD WINAPI ProcessClient(LPVOID);

int main(int argc, char *argv[])
{
	InitializeCriticalSection(&cs);

	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr,
		sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if(retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;	
	int size;

	while (1)
	{
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr,
			&addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			err_display("accept()");
			break;
		}


		_ClientInfo* ptr = AddClientInfo(client_sock, clientaddr);

		MatchPartner(ptr);

		HANDLE hThread = CreateThread(NULL, 0, ProcessClient, ptr, 0, NULL);
		if (hThread == NULL)
		{
			RemoveClientInfo(ptr);
			continue;
		}
		else
		{
			CloseHandle(hThread);
		}

	}

	// closesocket()
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}

DWORD WINAPI ProcessClient(LPVOID _ptr)
{
	_ClientInfo* ptr = (_ClientInfo*)_ptr;
	ptr->hWaitPart = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (ptr->hWaitPart == NULL) return 1;
	
	int size = PackPacket(ptr->buf, INTRO, INTRO_MSG);

	int retval = send(ptr->sock, ptr->buf, size, 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("intro send()");
		RemoveClientInfo(ptr);
		return -1;		
	}

	bool flag = false;
	int result;
	DWORD hRetval;

	while (1)
	{
		if (!PacketRecv(ptr->sock, ptr->buf))
		{
			break;
		}

		PROTOCOL protocol = GetProtocol(ptr->buf);

		switch (protocol)
		{
		case GAME_VALUE:
			UnPackPacket(ptr->buf, ptr->game_value);
			ptr->game_value_flag = true;

			SetEvent(ptr->hWaitPart);

			while (1)
			{
				if (ptr->part != NULL && ptr->part->game_value != NODATA)
				{
					result = who_win(ptr->game_value, ptr->part->game_value);

					switch (result)
					{
					case NO_WIN:
						ptr->no_win_flag = true;
						size = PackPacket(ptr->buf, GAME_RESULT, NO_WIN, NO_WIN_MSG);
						//ptr->game_value = NODATA;

						retval = send(ptr->sock, ptr->buf, size, 0);
						if (retval == SOCKET_ERROR)
						{
							err_display("game result send()");
						}
						
						SetEvent(ptr->hWaitPart);
						break;
					case WIN:
						size = PackPacket(ptr->buf, GAME_RESULT, WIN, WIN_MSG);
						retval = send(ptr->sock, ptr->buf, size, 0);
						if (retval == SOCKET_ERROR)
						{
							err_display("game result send()");
						}

						SetEvent(ptr->hWaitPart);
						break;
					case LOSE:
						size = PackPacket(ptr->buf, GAME_RESULT, LOSE, LOSE_MSG);
						retval = send(ptr->sock, ptr->buf, size, 0);
						if (retval == SOCKET_ERROR)
						{
							err_display("game result send()");
						}

						SetEvent(ptr->hWaitPart);
						break;
					}	// end switch
					break;
				}	// end if
				else
				{
					hRetval = WaitForSingleObject(ptr->part->hWaitPart, INFINITE);
					if (hRetval != WAIT_OBJECT_0) break;
					ResetEvent(ptr->hWaitPart);
				}
			}	// end while
			break;
		}		
		
		hRetval = WaitForSingleObject(ptr->part->hWaitPart, INFINITE);
		if (hRetval != WAIT_OBJECT_0) break;

		if (ptr->part->no_win_flag)
		{
			ptr->game_value = NODATA;
			ptr->game_value_flag = false;
			ptr->part->no_win_flag = false;
		}
		
	}//end while	

	if (!ptr->game_value_flag)
	{
		RemoveClientInfo(ptr);
		ExitThread(-1);
	}

	if (ptr->part->end_flag)
	{
		RemoveClientInfo(ptr->part);
		RemoveClientInfo(ptr);
	}
	else
	{
		ptr->end_flag = true;
	}
	
	return 0;

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
			return true;
		}
	}

	LeaveCriticalSection(&cs);

	return false;
}

void err_quit(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}


int who_win(int first, int second)
{
	if (first == second)
		return NO_WIN;
	else if (first % 3 == (second + 1) % 3)
		return WIN;
	else
		return LOSE;
}

bool PacketRecv(SOCKET _sock, char* _buf)
{
	int size;

	int retval = recvn(_sock, (char*)&size, sizeof(size), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recv error()");
		return false;
	}
	else if (retval == 0)
	{
		return false;
	}

	retval = recvn(_sock, _buf, size, 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recv error()");
		return false;

	}
	else if (retval == 0)
	{
		return false;
	}

	return true;
}

PROTOCOL GetProtocol(const char* _ptr)
{
	PROTOCOL protocol;
	memcpy(&protocol, _ptr, sizeof(PROTOCOL));

	return protocol;
}

int PackPacket(char* _buf, PROTOCOL _protocol, const char* _str1)
{
	int size = 0;
	char* ptr = _buf;
	int strsize1 = strlen(_str1);

	ptr = ptr + sizeof(size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	size = size + sizeof(_protocol);

	memcpy(ptr, &strsize1, sizeof(strsize1));
	ptr = ptr + sizeof(strsize1);
	size = size + sizeof(strsize1);

	memcpy(ptr, _str1, strsize1);
	ptr = ptr + strsize1;
	size = size + strsize1;

	ptr = _buf;
	memcpy(ptr, &size, sizeof(size));

	size = size + sizeof(size);

	return size;
}

int PackPacket(char* _buf, PROTOCOL _protocol, RESULT _result, const char* _str1)
{
	int size = 0;
	char* ptr = _buf;
	int strsize1 = strlen(_str1);

	ptr = ptr + sizeof(size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	size = size + sizeof(_protocol);

	memcpy(ptr, &_result, sizeof(_result));
	ptr = ptr + sizeof(_result);
	size = size + sizeof(_result);

	memcpy(ptr, &strsize1, sizeof(strsize1));
	ptr = ptr + sizeof(strsize1);
	size = size + sizeof(strsize1);

	memcpy(ptr, _str1, strsize1);
	ptr = ptr + strsize1;
	size = size + strsize1;

	ptr = _buf;
	memcpy(ptr, &size, sizeof(size));

	size = size + sizeof(size);

	return size;
}

void UnPackPacket(const char* _buf, int& _data)
{
	const char* ptr = _buf + sizeof(PROTOCOL);

	memcpy(&_data, ptr, sizeof(_data));
	ptr = ptr + sizeof(_data);

}

_ClientInfo* AddClientInfo(SOCKET _sock, SOCKADDR_IN _addr)
{
	EnterCriticalSection(&cs);

	_ClientInfo* ptr = new _ClientInfo;
	ZeroMemory(ptr, sizeof(_ClientInfo));
	ptr->sock = _sock;
	memcpy(&ptr->addr, &_addr, sizeof(SOCKADDR_IN));
	ptr->game_value = NODATA;
	ptr->part = nullptr;
	ptr->game_value_flag = false;
	ptr->no_win_flag = false;
	ptr->end_flag = false;

	ClientInfo[Count++] = ptr;

	LeaveCriticalSection(&cs);

	printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
		inet_ntoa(ptr->addr.sin_addr), ntohs(ptr->addr.sin_port));

	return ptr;
}

void RemoveClientInfo(_ClientInfo* _ptr)
{
	printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
		inet_ntoa(_ptr->addr.sin_addr), ntohs(_ptr->addr.sin_port));

	EnterCriticalSection(&cs);

	for (int i = 0; i < Count; i++)
	{
		if (ClientInfo[i] == _ptr)
		{
			closesocket(ClientInfo[i]->sock);
			if (ClientInfo[i]->part != NULL)
			{
				ClientInfo[i]->part->part = NULL;
			}
			delete ClientInfo[i];

			for (int j = i; j < Count - 1; j++)
			{
				ClientInfo[j] = ClientInfo[j + 1];
			}
			break;
		}
	}

	Count--;

	LeaveCriticalSection(&cs);
}
