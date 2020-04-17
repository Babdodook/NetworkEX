#pragma warning(disable : 4996)
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <string>

#define SERVERPORT 9000
#define BUFSIZE    4096
#define SIZE 10
#define MAX_COUNT 100		// 유저, 클라이언트, 스레드 배열 최대 카운트
#define FILENAMESIZE 256	// 파일이름 사이즈
#define MAX_FILESIZE 38000	// 전송가능한 파일 최대 크기
#define MAX_NAMESIZE 15		// 파일 이름 최대 크기

#define MSG_WORNG_ID "잘못된 아이디입니다."
#define MSG_WRONG_PW "잘못된 비밀번호입니다."
#define MSG_INUSE "이미 로그인된 사용자입니다."
#define MSG_SUCCESS "로그인 되었습니다."

#define MSG_EXIST_ID "이미 존재하는 아이디입니다."
#define MSG_ENABLED "회원가입 되었습니다."

#define FILE_EXIST_MSG "전송하고자 하는 파일은 이미 서버에 존재하는 파일입니다."
#define FILE_NAMESIZE_DENY_MSG "전송하고자 하는 파일의 이름 크기가 큽니다."
#define FILE_SIZE_DENY_MSG "전송하고자 하는 파일의 크기가 큽니다."

enum PROTOCOL		// 메뉴를 나누기 위한 프로토콜
{
	MAINMENU=1,		// 메인메뉴
	LOGIN,			// 로그인
	REGISTER,		// 회원가입
	EXIT,			// 종료
	LOGINMENU,		// 로그인메뉴
	FILE_UPLOAD,	// 파일전송
	LOGOUT,			// 로그아웃
	DELETEACC,		// 회원탈퇴
};

enum STATE			// 초기화, 메뉴화면, 로그인화면 상태
{
	INIT_STATE=1,
	MAINMENU_STATE,
	LOGINMENU_STATE,
};

enum LOGIN_CODE		// 로그인 결과 코드
{
	WORNG_ID=1,		//	아이디 다름
	WORNG_PW,		//	비밀번호 다름
	INUSE,			//	이미 로그인 중
	SUCCESS			//	로그인 성공
};

enum REG_CODE		// 회원가입 결과 코드
{
	EXIST_ID=1,		//	존재하는 아이디
	ENABLED,		//	생성가능
};

enum FILE_CODE		// 파일 전송시 사용되는 프로토콜
{
	FILE_INFO=1,				// 파일의 정보
	FILE_TRANS_DENY,			// 파일 전송 거부
	FILE_TRANS_START_POINT,		// 파일 전송 시작지점
	FILE_TRANS_DATA,			// 파일 전송
};

enum DENY_CODE		// 파일 전송 거부 코드
{
	FILEEXIST = -1,		// 파일이 이미 존재
	NAMESIZE_DENY = 1,	// 이름 크기 제한
	SIZE_DENY			// 파일 크기 제한
};

struct _File_info					// 파일 정보
{
	char filename[FILENAMESIZE];
	int  filesize;
	int  nowsize;
};

typedef struct user					// 유저 정보
{
	char id[SIZE];
	char pw[SIZE];
	bool isLogin;	// 현재 로그인 중인지?
}_UserInfo;

typedef struct clientinfo			// 클라이언트 정보
{
	SOCKET sock;
	SOCKADDR_IN addr;
	char buf[BUFSIZE];
	_UserInfo *user;			// 유저 정보를 받기 위한 것
	STATE state;				// 상태
	_File_info file_info;		// 파일 정보를 저장하기 위해
	HANDLE wait_upload_event;	// 파일 업로드 순서를 따지기위한 이벤트
	HANDLE hthread;				// 나를 담당하는 스레드
	bool isUploading;			// 업로드 중인지 판별하기 위한 플래그
}_ClientInfo;

_UserInfo* UserInfoArr[MAX_COUNT];		// 유저 정보들
										// 프로그램 시작 시 userDatabase.txt에 있는 정보를 받는다
_ClientInfo* ClientInfo[MAX_COUNT];		// 클라이언트 정보들
int userCount = 0;
int clientCount = 0;

HANDLE hThread[MAX_COUNT];		// 현재 생성된 스레드들
int ThreadCount = 0;

CRITICAL_SECTION cs;

void err_quit(const char *msg);
void err_display(const char *msg);
int recvn(SOCKET s, char *buf, int len, int flags);
SOCKET sock_init();

bool PacketRecv(SOCKET _sock, char* _buf);
PROTOCOL GetProtocol(const char* _ptr);
FILE_CODE GetFilecode(const char* _ptr);		// 파일코드 읽기위함

// 유저관련 기능
void LoadUserInfo(const char* fileName);			// 유저 정보 불러오기
void SaveUserInfo(const char* fileName);			// 유저 정보 저장하기
int SearchLogin(_ClientInfo* ptr, _UserInfo user);	// 로그인할때 유저 검색
int SearchReg(_UserInfo user);						// 회원가입할때 유저 검색
void AddUserInfo(_UserInfo user);					// 회원가입 기능
void RemoveUserInfo(_UserInfo* user);				// 유저 정보 삭제(회원탈퇴)

_ClientInfo * AddClientInfo(SOCKET _sock, SOCKADDR_IN _addr);	// 클라이언트 추가
void RemoveClientInfo(_ClientInfo* _ptr);						// 클라이언트 삭제
_ClientInfo* SearchClientInfo(HANDLE _hthread);					// 해당 스레드를 가진 클라이언트 검색

bool AddThread(LPTHREAD_START_ROUTINE process, _ClientInfo* _ptr);	// 스레드 추가
void RemoveThread(HANDLE _hthread);									// 스레드 삭제

// 팩킹
int Pack_mainMenu(char* _buf, PROTOCOL _protocol, LOGIN_CODE code, const char* msg);
int Pack_mainMenu(char* _buf, PROTOCOL _protocol, REG_CODE code, const char* msg);
int Pack_transDeny(char* _buf, FILE_CODE  _protocol, int _data, const char* _str);
int Pack_transStartPoint(char* _buf, FILE_CODE _protocol, int _data);

// 언팩킹
void UnPack_userInfo(const char* _buf, _UserInfo *user);
void UnPack_fileinfo(const char* _buf, char* _str1, int& _data1);
void UnPack_transData(const char* _buf, int& _size, char* _targetbuf);

bool SearchFile(const char *filename);

DWORD WINAPI ProcessClient(LPVOID);	// 클라이언트 담당 스레드
DWORD WINAPI RemoveClient(LPVOID);	// 클라이언트 및 스레드 삭제 스레드

int main(int argc, char *argv[])
{
	LoadUserInfo("userDatabase.txt");		// 유저 정보 불러오기

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;
	InitializeCriticalSection(&cs);
	// 소켓 초기화
	SOCKET listen_sock = sock_init();

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	int size;

	// 0번째에 이벤트 넣기, ThreadCount 갱신하기 위함
	hThread[ThreadCount++] = CreateEvent(nullptr, false, false, nullptr);

	CreateThread(nullptr, 0, RemoveClient, nullptr, 0, nullptr);

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

		AddThread(ProcessClient, ptr);

		SetEvent(hThread[0]);
	}

	closesocket(listen_sock);
	DeleteCriticalSection(&cs);

	WSACleanup();
	return 0;
}

bool AddThread(LPTHREAD_START_ROUTINE process, _ClientInfo* _ptr)
{
	EnterCriticalSection(&cs);
	_ptr->hthread = CreateThread(nullptr, 0, process, _ptr, 0, nullptr);
	if (_ptr->hthread == nullptr)
	{
		LeaveCriticalSection(&cs);
		return false;
	}

	hThread[ThreadCount++] = _ptr->hthread;
	LeaveCriticalSection(&cs);
	return true;
}

void RemoveThread(HANDLE _hthread)
{
	EnterCriticalSection(&cs);

	for (int i = 1; i < ThreadCount; i++)
	{
		if (hThread[i] == _hthread)
		{
			CloseHandle(_hthread);

			for (int j = i; j < ThreadCount - 1; j++)
			{
				hThread[j] = hThread[j + 1];
			}

			hThread[ThreadCount - 1] = nullptr;
			ThreadCount--;
			break;
		}
	}

	LeaveCriticalSection(&cs);
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

SOCKET sock_init()
{
	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	int retval = bind(listen_sock, (SOCKADDR *)&serveraddr,
		sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	return listen_sock;
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

FILE_CODE GetFilecode(const char* _ptr)
{
	FILE_CODE fcode;
	memcpy(&fcode, _ptr, sizeof(FILE_CODE));

	return fcode;
}

void LoadUserInfo(const char* fileName)
{
	std::ifstream fileStream;
	fileStream.open(fileName);

	if (!fileStream.is_open())
	{
		printf("파일 열기 실패\n");
		throw std::bad_exception();
	}
	
	// Read data
	std::string data = "";

	while (std::getline(fileStream, data))
	{
		UserInfoArr[userCount] = new _UserInfo;

		std::string splitData = data;
		std::string id, pw;

		id = splitData.substr(0, splitData.find(" "));
		strcpy(UserInfoArr[userCount]->id, id.c_str());

		splitData = splitData.substr(splitData.find(" ") + 1);

		pw = splitData;
		strcpy(UserInfoArr[userCount]->pw, pw.c_str());

		UserInfoArr[userCount]->isLogin = false;
		
		userCount++;
	}

	fileStream.close();
}

void SaveUserInfo(const char* fileName)
{
	std::ofstream fileStream;
	fileStream.open(fileName);

	for (int i = 0; i < userCount; i++)
	{
		fileStream << UserInfoArr[i]->id << " " << UserInfoArr[i]->pw << std::endl;
	}

	fileStream.close();
}

_ClientInfo * AddClientInfo(SOCKET _sock, SOCKADDR_IN _addr)
{
	EnterCriticalSection(&cs);
	_ClientInfo* ptr = new _ClientInfo;
	ptr->sock = _sock;
	memcpy(&ptr->addr, &_addr, sizeof(SOCKADDR_IN));
	ptr->user = nullptr;
	ptr->state = INIT_STATE;
	ptr->isUploading = false;
	ZeroMemory(&ptr->file_info, sizeof(_File_info));
	ptr->wait_upload_event = CreateEvent(nullptr, false, false, nullptr);

	ClientInfo[clientCount++] = ptr;

	LeaveCriticalSection(&cs);

	printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
		inet_ntoa(ptr->addr.sin_addr), ntohs(ptr->addr.sin_port));

	return ptr;
}

void RemoveClientInfo(_ClientInfo* _ptr)
{
	EnterCriticalSection(&cs);

	for (int i = 0; i < clientCount; i++)
	{
		if (ClientInfo[i] == _ptr)
		{
			closesocket(ClientInfo[i]->sock);

			delete ClientInfo[i];

			for (int j = i; j < clientCount - 1; j++)
			{
				ClientInfo[j] = ClientInfo[j + 1];
			}
			ClientInfo[clientCount - 1] = nullptr;
			clientCount--;

			printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
				inet_ntoa(_ptr->addr.sin_addr), ntohs(_ptr->addr.sin_port));

			break;
		}
	}

	LeaveCriticalSection(&cs);
}

// 팩킹 함수 정의
int Pack_mainMenu(char* _buf, PROTOCOL _protocol, LOGIN_CODE code, const char* msg)
{
	int size = 0;
	char* ptr = _buf;
	int strsize = strlen(msg);

	ptr = ptr + sizeof(size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	size = size + sizeof(_protocol);

	memcpy(ptr, &code, sizeof(LOGIN_CODE));
	ptr = ptr + sizeof(LOGIN_CODE);
	size = size + sizeof(LOGIN_CODE);

	memcpy(ptr, &strsize, sizeof(strsize));
	ptr = ptr + sizeof(strsize);
	size = size + sizeof(strsize);

	memcpy(ptr, msg, strsize);
	ptr = ptr + strsize;
	size = size + strsize;

	ptr = _buf;
	memcpy(ptr, &size, sizeof(size));

	size = size + sizeof(size);

	return size;
}

int Pack_mainMenu(char* _buf, PROTOCOL _protocol, REG_CODE code, const char* msg)
{
	int size = 0;
	char* ptr = _buf;
	int strsize = strlen(msg);

	ptr = ptr + sizeof(size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	size = size + sizeof(_protocol);

	memcpy(ptr, &code, sizeof(REG_CODE));
	ptr = ptr + sizeof(REG_CODE);
	size = size + sizeof(REG_CODE);

	memcpy(ptr, &strsize, sizeof(strsize));
	ptr = ptr + sizeof(strsize);
	size = size + sizeof(strsize);

	memcpy(ptr, msg, strsize);
	ptr = ptr + strsize;
	size = size + strsize;

	ptr = _buf;
	memcpy(ptr, &size, sizeof(size));

	size = size + sizeof(size);

	return size;
}

int Pack_transDeny(char* _buf, FILE_CODE  _protocol, int _data, const char* _str)
{
	char* ptr = _buf;
	int strsize = strlen(_str);
	int size = 0;

	ptr = ptr + sizeof(size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	size = size + sizeof(_protocol);

	memcpy(ptr, &_data, sizeof(_data));
	ptr = ptr + sizeof(_data);
	size = size + sizeof(_data);

	memcpy(ptr, &strsize, sizeof(strsize));
	ptr = ptr + sizeof(strsize);
	size = size + sizeof(strsize);

	memcpy(ptr, _str, strsize);
	ptr = ptr + strsize;
	size = size + strsize;

	ptr = _buf;
	memcpy(ptr, &size, sizeof(size));

	size = size + sizeof(size);
	return size;
}

int Pack_transStartPoint(char* _buf, FILE_CODE _protocol, int _data)
{
	char* ptr = _buf;
	int size = 0;

	ptr = ptr + sizeof(size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	size = size + sizeof(_protocol);

	memcpy(ptr, &_data, sizeof(_data));
	ptr = ptr + sizeof(_data);
	size = size + sizeof(_data);

	ptr = _buf;
	memcpy(ptr, &size, sizeof(size));

	size = size + sizeof(size);
	return size;
}


// 언팩킹 함수 정의
void UnPack_userInfo(const char* _buf, _UserInfo *user)
{
	const char* ptr = _buf + sizeof(PROTOCOL);
	int strsize;

	memcpy(&strsize, ptr, sizeof(int));
	ptr = ptr + sizeof(int);

	memcpy(user->id, ptr, strsize);
	ptr = ptr + strsize;

	memcpy(&strsize, ptr, sizeof(int));
	ptr = ptr + sizeof(int);

	memcpy(user->pw, ptr, strsize);
	ptr = ptr + strsize;
	
}

void UnPack_fileinfo(const char* _buf, char* _str1, int& _data1)
{
	const char* ptr = _buf + sizeof(PROTOCOL);
	int strsize;

	memcpy(&strsize, ptr, sizeof(strsize));
	ptr = ptr + sizeof(strsize);

	memcpy(_str1, ptr, strsize);
	ptr = ptr + strsize;

	memcpy(&_data1, ptr, sizeof(_data1));
	ptr = ptr + sizeof(_data1);
}

void UnPack_transData(const char* _buf, int& _size, char* _targetbuf)
{
	const char* ptr = _buf + sizeof(PROTOCOL);

	memcpy(&_size, ptr, sizeof(_size));
	ptr = ptr + sizeof(_size);

	memcpy(_targetbuf, ptr, _size);
}

int SearchReg(_UserInfo user)
{
	for (int i = 0; i < userCount; i++)
	{
		if (strcmp(UserInfoArr[i]->id, user.id) == 0)
		{
			return REG_CODE::EXIST_ID;
		}
	}

	return REG_CODE::ENABLED;
}

int SearchLogin(_ClientInfo* ptr, _UserInfo user)
{
	for (int i = 0; i < userCount; i++)
	{
		if (strcmp(UserInfoArr[i]->id, user.id) == 0
			&& strcmp(UserInfoArr[i]->pw, user.pw) == 0)
		{
			if (UserInfoArr[i]->isLogin)
				return LOGIN_CODE::INUSE;
			else
			{
				UserInfoArr[i]->isLogin = true;
				ptr->user = UserInfoArr[i];
				return LOGIN_CODE::SUCCESS;
			}
		}
		else if (strcmp(UserInfoArr[i]->id, user.id)
			&& strcmp(UserInfoArr[i]->pw, user.pw) == 0)
		{
			return LOGIN_CODE::WORNG_ID;
		}
		else if (strcmp(UserInfoArr[i]->id, user.id) == 0
			&& strcmp(UserInfoArr[i]->pw, user.pw))
		{
			return LOGIN_CODE::WORNG_PW;
		}
	}

	return LOGIN_CODE::WORNG_ID;
}

void AddUserInfo(_UserInfo user)
{
	EnterCriticalSection(&cs);

	UserInfoArr[userCount] = new _UserInfo;
	strcpy(UserInfoArr[userCount]->id, user.id);
	strcpy(UserInfoArr[userCount]->pw, user.pw);
	UserInfoArr[userCount]->isLogin = false;
	userCount++;

	SaveUserInfo("userDatabase.txt");

	LeaveCriticalSection(&cs);
}

void RemoveUserInfo(_UserInfo* user)
{
	EnterCriticalSection(&cs);

	for (int i = 0; i < userCount; i++)
	{
		if (UserInfoArr[i] == user)
		{
			delete UserInfoArr[i];

			for (int j = i; j < userCount - 1; j++)
			{
				UserInfoArr[j] = UserInfoArr[j + 1];
			}
			UserInfoArr[userCount - 1] = nullptr;
			userCount--;

			break;
		}
	}

	SaveUserInfo("userDatabase.txt");

	LeaveCriticalSection(&cs);
}

bool SearchFile(const char *filename)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFindFile = FindFirstFile(filename, &FindFileData);
	if (hFindFile == INVALID_HANDLE_VALUE)
		return false;
	else {
		FindClose(hFindFile);
		return true;
	}
}

DWORD WINAPI ProcessClient(LPVOID _ptr)
{
	_ClientInfo* ptr = (_ClientInfo*)_ptr;

	PROTOCOL protocol;
	int size;
	int retval;
	char msg[BUFSIZE];
	ZeroMemory(msg, sizeof(msg));

	_UserInfo user;
	ZeroMemory(user.id, sizeof(user.id));
	ZeroMemory(user.pw, sizeof(user.pw));

	bool endflag = false;

	while (1)
	{
		switch (ptr->state)
		{
		case INIT_STATE:						// 초기화 상태
			ptr->state = MAINMENU_STATE;		// 메인메뉴 상태로 이동
			break;
		case MAINMENU_STATE:					// 메인메뉴 상태
			if (!PacketRecv(ptr->sock, ptr->buf))
			{
				endflag = true;
				break;
			}

			protocol = GetProtocol(ptr->buf);

			UnPack_userInfo(ptr->buf, &user);	// 입력받은 유저의 정보를 받아온다
			LOGIN_CODE login_code;
			REG_CODE reg_code;
			int check;

			switch (protocol)
			{
			case LOGIN:								// 로그인이라면?
				check = SearchLogin(ptr, user);		// 로그인의 경우에 맞춰서 결과 판별해준다
													// 결과에 맞는 메시지도 셋팅해준다
				protocol = LOGIN;

				switch (check)
				{
				case LOGIN_CODE::WORNG_ID:				// 아이디 틀림
					strcpy(msg, MSG_WORNG_ID);
					login_code = LOGIN_CODE::WORNG_ID;
					break;
				case LOGIN_CODE::WORNG_PW:				// 비밀번호 틀림
					strcpy(msg, MSG_WRONG_PW);
					login_code = LOGIN_CODE::WORNG_PW;
					break;
				case LOGIN_CODE::INUSE:					// 이미 로그인중
					strcpy(msg, MSG_INUSE);
					login_code = LOGIN_CODE::INUSE;
					break;
				case LOGIN_CODE::SUCCESS:				// 로그인 성공
					strcpy(msg, MSG_SUCCESS);
					login_code = LOGIN_CODE::SUCCESS;
					ptr->state = LOGINMENU_STATE;		// 로그인 성공했으니 로그인메뉴 상태로 간다
					break;
				}
				size = Pack_mainMenu(ptr->buf, protocol, login_code, msg);

				break;
			case REGISTER:							// 회원가입의 경우도 로그인과 동일함
				check = SearchReg(user);
				protocol = REGISTER;

				switch (check)
				{
				case REG_CODE::EXIST_ID:			// 이미 존재하는 아이디
					strcpy(msg, MSG_EXIST_ID);
					reg_code = REG_CODE::EXIST_ID;
					break;
				case REG_CODE::ENABLED:				// 회원가입 가능
					AddUserInfo(user);				// 입력받은 정보로 회원가입

					strcpy(msg, MSG_ENABLED);
					reg_code = REG_CODE::ENABLED;
					break;
				}
				size = Pack_mainMenu(ptr->buf, protocol, reg_code, msg);

				break;
			}

			retval = send(ptr->sock, ptr->buf, size, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("mainMenu MSG send()");
				break;
			}

			break;
		case LOGINMENU_STATE:						// 로그인에 성공한 상태
			while (1)
			{
				bool escapeflag = false;
				char buf[BUFSIZE];
				bool fileflag = false;
				bool sendfail = false;
				bool file_escapeflag = false;
				FILE* fp = nullptr;

				if (!PacketRecv(ptr->sock, ptr->buf))	// 로그인메뉴에서 선택한 메뉴를 받는다
														// 파일전송, 로그아웃, 회원탈퇴
				{
					endflag = true;
					break;
				}

				protocol = GetProtocol(ptr->buf);

				switch (protocol)
				{
				case FILE_UPLOAD:						// 파일 전송
					while (1)
					{
						if (!PacketRecv(ptr->sock, ptr->buf))
						{
							escapeflag = true;
							break;
						}
						FILE_CODE fcode = GetFilecode(ptr->buf);	

						switch (fcode)
						{
						case FILE_INFO:						// 파일 정보 받기
							char filename[FILENAMESIZE];
							int filesize;
							memset(filename, 0, sizeof(filename));

							UnPack_fileinfo(ptr->buf, filename, filesize);		// 파일 이름과 크기를 언팩킹

							strcpy(ptr->file_info.filename, filename);			// 이름을 셋팅해준다
							// 같은 파일을 업로드 중인 클라이언트가 있을때, 기다린다.
							for (int i = 0; i < clientCount; i++)
							{
								if (ptr != ClientInfo[i])
								{
									// 다른 클라이언트가 전송하려는 파일 이름이 같고, 전송중이면?
									if (strcmp(ptr->file_info.filename, ClientInfo[i]->file_info.filename) == 0
										&& ClientInfo[i]->isUploading)
									{
										// 이벤트를 켜주길 기다린다
										WaitForSingleObject(ptr->wait_upload_event, INFINITE);
										break; // 한명씩 처리되도록 브레이크
									}
								}
							}

							if (strlen(filename) > MAX_NAMESIZE)	// 파일 이름이 제한되는가?
							{										// 전송 거부 코드 보내기
								int size = Pack_transDeny(ptr->buf, FILE_TRANS_DENY, NAMESIZE_DENY, FILE_NAMESIZE_DENY_MSG);
								sendfail = true;
								file_escapeflag = true;

								retval = send(ptr->sock, ptr->buf, size, 0);
								if (retval == SOCKET_ERROR)
								{
									err_display("file trans deny send()");
								}

								printf("전송 거부 - 파일 이름 크기 제한\n");

								break;
							}
							else if (filesize > MAX_FILESIZE)	// 파일 크기가 제한되는가?
							{									// 전송 거부 코드 보내기
								int size = Pack_transDeny(ptr->buf, FILE_TRANS_DENY, SIZE_DENY, FILE_SIZE_DENY_MSG);
								sendfail = true;
								file_escapeflag = true;

								retval = send(ptr->sock, ptr->buf, size, 0);
								if (retval == SOCKET_ERROR)
								{
									err_display("file trans deny send()");
								}

								printf("전송 거부 - 파일 크기 제한\n");

								break;
							}
							else //	파일 이름과 크기는 문제 없음
							{
								if (SearchFile(filename))		// 이미 존재하는 파일인가?
								{
									fp = fopen(filename, "rb");
									fseek(fp, 0, SEEK_END);
									int fsize = ftell(fp);		// 현재 가진 파일의 크기
									fclose(fp);
									if (filesize == fsize)		// 받으려는 파일 크기가 현재 가진 파일과 같다면?
																// 이미 있는 파일이므로 전송 거부
									{
										printf("전송 거부 - 존재하는 파일 전송 요구\n");

										int size = Pack_transDeny(ptr->buf, FILE_TRANS_DENY, FILEEXIST, FILE_EXIST_MSG);
										sendfail = true;
										file_escapeflag = true;

										retval = send(ptr->sock, ptr->buf, size, 0);
										if (retval == SOCKET_ERROR)
										{
											err_display("file trans deny send()");
										}
										break;
									}
									else // 파일크기가 다른 경우엔 이어서 받도록 한다
									{
										strcpy(ptr->file_info.filename, filename);
										ptr->file_info.filesize = filesize;
										ptr->file_info.nowsize = fsize;
									}

								}
								else //	그것도 아니면 처음부터 받는다
								{
									strcpy(ptr->file_info.filename, filename);
									ptr->file_info.filesize = filesize;
									ptr->file_info.nowsize = 0;
								}

								// 파일을 보내야할 시작 지점 정보 보내기
								size = Pack_transStartPoint(ptr->buf, FILE_TRANS_START_POINT, ptr->file_info.nowsize);

								retval = send(ptr->sock, ptr->buf, size, 0);
								if (retval == SOCKET_ERROR)
								{
									err_display("file trans start point send()");
									break;
								}
							}
							break;
						case FILE_TRANS_DATA:	// 파일 받기
							if (!fileflag)		// 처음에는 fileflag가 false다 (한번만 열기 위해)
							{
								fileflag = true;							// true로 바꿔주고
								fp = fopen(ptr->file_info.filename, "ab");	// 파일을 열어준다 
								ptr->isUploading = true;					// 이때 내가 업로드 중이라는걸 표시하기 위해
																			// isUploading을 true로 바꿔준다
							}

							// 대략 파일 받는 부분..
							int transsize;
							UnPack_transData(ptr->buf, transsize, buf);
							fwrite(buf, 1, transsize, fp);
							if (ferror(fp)) {
								perror("파일 입출력 오류");
								file_escapeflag = true;
								break;
							}
							ptr->file_info.nowsize += transsize;	// 현재 크기에 받은 크기만큼 계속 더해준다
							break;
						}

						// 파일크기가 0이 아니고, 받으려했던 파일 크기와 현재 파일 크기가 같다면?
						// 전송 완료된 것임
						if (ptr->file_info.filesize != 0 && ptr->file_info.filesize == ptr->file_info.nowsize)
						{
							printf("전송완료\n");
							fclose(fp);

							file_escapeflag = true;
							ptr->isUploading = false;

							// 전송이 끝난 뒤, 기다리는 클라이언트의 이벤트를 켜준다
							for (int i = 0; i < clientCount; i++)
							{
								if (ptr != ClientInfo[i])
								{
									// 내가 전송한 파일 이름과 상대가 전송하려는 파일 이름이 같고,
									// 상대는 업로드중이 아니라면?
									if (strcmp(ptr->file_info.filename, ClientInfo[i]->file_info.filename) == 0
										&& !ClientInfo[i]->isUploading)
									{
										// 이벤트를 켜준다
										SetEvent(ClientInfo[i]->wait_upload_event);
										break;	// 한명씩 켜주기 위해 브레이크
									}
								}
							}

							ZeroMemory(&ptr->file_info, sizeof(_File_info));
						}
						else if (sendfail)
						{
							sendfail = false;
							printf("전송실패\n");
						}

						if (file_escapeflag)
							break;
					}//end while
					break;
				case LOGOUT:					// 로그아웃
					escapeflag = true;
					ptr->user->isLogin = false;	// 로그아웃이니 false로 바꾼다
					ptr->user = nullptr;
					break;
				case DELETEACC:					// 회원탈퇴
					escapeflag = true;
					ptr->user->isLogin = false;
					RemoveUserInfo(ptr->user);	// 회원 삭제 기능 수행
					break;
				}

				if (escapeflag)
				{
					ptr->state = MAINMENU_STATE;	// 메인메뉴로 돌아간다
					break;
				}

				if (endflag)
				{
					break;
				}
			}// end while
			break;
		}// end state switch

		if (endflag)
			break;
	}// end while

	return 0;
}

DWORD WINAPI RemoveClient(LPVOID _ptr)			// 클라이언트 및 스레드 삭제
{
	while (1)
	{
		int index = WaitForMultipleObjects(ThreadCount, hThread, false, INFINITE);	// 스레드 종료를 기다린다
		index -= WAIT_OBJECT_0;
		EnterCriticalSection(&cs);
		if (index == 0)
		{
			LeaveCriticalSection(&cs);
			continue;
		}

		_ClientInfo* ptr = SearchClientInfo(hThread[index]);		// 스레드가 담당하던 클라이언트를 찾는다
		if (ptr == nullptr)					// 클라이언트가 없으면?
		{
			RemoveThread(hThread[index]);	// 바로 삭제 후
			LeaveCriticalSection(&cs);		// 키 반납
			continue;
		}

		RemoveThread(hThread[index]);		// 스레드 먼저 삭제

		ptr->hthread = nullptr;

		switch (ptr->state)					// 클라이언트의 마지막 상태에 따라서 삭제 진행
		{
		case INIT_STATE:
		case MAINMENU_STATE:
		case LOGINMENU_STATE:				// 체크해야 하는건 업로드중인지만 체크하면 되기 때문에
											// 업로드 상태 체크해준다
			if (ptr->isUploading)			// 종료했을때 업로드 중이었다면?
											// 대기 중이던 클라이언트를 켜줘야 한다
			{
				for (int i = 0; i < clientCount; i++)
				{
					if (ptr != ClientInfo[i])	// 나를 제외하고
					{
						// 파일 이름이 동일하고 업로드중이지 않은 클라이언트를 찾으면? (기다리고 있는 클라이언트)
						if (strcmp(ptr->file_info.filename, ClientInfo[i]->file_info.filename) == 0
							&& !ClientInfo[i]->isUploading)
						{
							// 이벤트를 켜준다
							SetEvent(ClientInfo[i]->wait_upload_event);
							break;	// 한명씩 대기 풀기위해 브레이크
						}
					}
				}
			}
			if(ptr->user != nullptr)		// 유저 정보가 있다면?
				ptr->user->isLogin = false;	// 로그아웃 시켜준다. (다른 클라이언트가 해당 아이디로 로그인할 수 있도록)
			RemoveClientInfo(ptr);			// 작업완료후 클라이언트 삭제 진행
			break;
		}

		LeaveCriticalSection(&cs);
	}
}

_ClientInfo* SearchClientInfo(HANDLE _hthread)
{
	EnterCriticalSection(&cs);

	_ClientInfo* ptr = nullptr;

	for (int i = 0; i < clientCount; i++)
	{
		if (ClientInfo[i]->hthread == _hthread)
		{
			ptr = ClientInfo[i];
			break;
		}
	}

	LeaveCriticalSection(&cs);
	return ptr;
}