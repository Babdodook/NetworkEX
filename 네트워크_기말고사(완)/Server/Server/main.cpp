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
#define MAX_COUNT 100		// ����, Ŭ���̾�Ʈ, ������ �迭 �ִ� ī��Ʈ
#define FILENAMESIZE 256	// �����̸� ������
#define MAX_FILESIZE 38000	// ���۰����� ���� �ִ� ũ��
#define MAX_NAMESIZE 15		// ���� �̸� �ִ� ũ��

#define MSG_WORNG_ID "�߸��� ���̵��Դϴ�."
#define MSG_WRONG_PW "�߸��� ��й�ȣ�Դϴ�."
#define MSG_INUSE "�̹� �α��ε� ������Դϴ�."
#define MSG_SUCCESS "�α��� �Ǿ����ϴ�."

#define MSG_EXIST_ID "�̹� �����ϴ� ���̵��Դϴ�."
#define MSG_ENABLED "ȸ������ �Ǿ����ϴ�."

#define FILE_EXIST_MSG "�����ϰ��� �ϴ� ������ �̹� ������ �����ϴ� �����Դϴ�."
#define FILE_NAMESIZE_DENY_MSG "�����ϰ��� �ϴ� ������ �̸� ũ�Ⱑ Ů�ϴ�."
#define FILE_SIZE_DENY_MSG "�����ϰ��� �ϴ� ������ ũ�Ⱑ Ů�ϴ�."

enum PROTOCOL		// �޴��� ������ ���� ��������
{
	MAINMENU=1,		// ���θ޴�
	LOGIN,			// �α���
	REGISTER,		// ȸ������
	EXIT,			// ����
	LOGINMENU,		// �α��θ޴�
	FILE_UPLOAD,	// ��������
	LOGOUT,			// �α׾ƿ�
	DELETEACC,		// ȸ��Ż��
};

enum STATE			// �ʱ�ȭ, �޴�ȭ��, �α���ȭ�� ����
{
	INIT_STATE=1,
	MAINMENU_STATE,
	LOGINMENU_STATE,
};

enum LOGIN_CODE		// �α��� ��� �ڵ�
{
	WORNG_ID=1,		//	���̵� �ٸ�
	WORNG_PW,		//	��й�ȣ �ٸ�
	INUSE,			//	�̹� �α��� ��
	SUCCESS			//	�α��� ����
};

enum REG_CODE		// ȸ������ ��� �ڵ�
{
	EXIST_ID=1,		//	�����ϴ� ���̵�
	ENABLED,		//	��������
};

enum FILE_CODE		// ���� ���۽� ���Ǵ� ��������
{
	FILE_INFO=1,				// ������ ����
	FILE_TRANS_DENY,			// ���� ���� �ź�
	FILE_TRANS_START_POINT,		// ���� ���� ��������
	FILE_TRANS_DATA,			// ���� ����
};

enum DENY_CODE		// ���� ���� �ź� �ڵ�
{
	FILEEXIST = -1,		// ������ �̹� ����
	NAMESIZE_DENY = 1,	// �̸� ũ�� ����
	SIZE_DENY			// ���� ũ�� ����
};

struct _File_info					// ���� ����
{
	char filename[FILENAMESIZE];
	int  filesize;
	int  nowsize;
};

typedef struct user					// ���� ����
{
	char id[SIZE];
	char pw[SIZE];
	bool isLogin;	// ���� �α��� ������?
}_UserInfo;

typedef struct clientinfo			// Ŭ���̾�Ʈ ����
{
	SOCKET sock;
	SOCKADDR_IN addr;
	char buf[BUFSIZE];
	_UserInfo *user;			// ���� ������ �ޱ� ���� ��
	STATE state;				// ����
	_File_info file_info;		// ���� ������ �����ϱ� ����
	HANDLE wait_upload_event;	// ���� ���ε� ������ ���������� �̺�Ʈ
	HANDLE hthread;				// ���� ����ϴ� ������
	bool isUploading;			// ���ε� ������ �Ǻ��ϱ� ���� �÷���
}_ClientInfo;

_UserInfo* UserInfoArr[MAX_COUNT];		// ���� ������
										// ���α׷� ���� �� userDatabase.txt�� �ִ� ������ �޴´�
_ClientInfo* ClientInfo[MAX_COUNT];		// Ŭ���̾�Ʈ ������
int userCount = 0;
int clientCount = 0;

HANDLE hThread[MAX_COUNT];		// ���� ������ �������
int ThreadCount = 0;

CRITICAL_SECTION cs;

void err_quit(const char *msg);
void err_display(const char *msg);
int recvn(SOCKET s, char *buf, int len, int flags);
SOCKET sock_init();

bool PacketRecv(SOCKET _sock, char* _buf);
PROTOCOL GetProtocol(const char* _ptr);
FILE_CODE GetFilecode(const char* _ptr);		// �����ڵ� �б�����

// �������� ���
void LoadUserInfo(const char* fileName);			// ���� ���� �ҷ�����
void SaveUserInfo(const char* fileName);			// ���� ���� �����ϱ�
int SearchLogin(_ClientInfo* ptr, _UserInfo user);	// �α����Ҷ� ���� �˻�
int SearchReg(_UserInfo user);						// ȸ�������Ҷ� ���� �˻�
void AddUserInfo(_UserInfo user);					// ȸ������ ���
void RemoveUserInfo(_UserInfo* user);				// ���� ���� ����(ȸ��Ż��)

_ClientInfo * AddClientInfo(SOCKET _sock, SOCKADDR_IN _addr);	// Ŭ���̾�Ʈ �߰�
void RemoveClientInfo(_ClientInfo* _ptr);						// Ŭ���̾�Ʈ ����
_ClientInfo* SearchClientInfo(HANDLE _hthread);					// �ش� �����带 ���� Ŭ���̾�Ʈ �˻�

bool AddThread(LPTHREAD_START_ROUTINE process, _ClientInfo* _ptr);	// ������ �߰�
void RemoveThread(HANDLE _hthread);									// ������ ����

// ��ŷ
int Pack_mainMenu(char* _buf, PROTOCOL _protocol, LOGIN_CODE code, const char* msg);
int Pack_mainMenu(char* _buf, PROTOCOL _protocol, REG_CODE code, const char* msg);
int Pack_transDeny(char* _buf, FILE_CODE  _protocol, int _data, const char* _str);
int Pack_transStartPoint(char* _buf, FILE_CODE _protocol, int _data);

// ����ŷ
void UnPack_userInfo(const char* _buf, _UserInfo *user);
void UnPack_fileinfo(const char* _buf, char* _str1, int& _data1);
void UnPack_transData(const char* _buf, int& _size, char* _targetbuf);

bool SearchFile(const char *filename);

DWORD WINAPI ProcessClient(LPVOID);	// Ŭ���̾�Ʈ ��� ������
DWORD WINAPI RemoveClient(LPVOID);	// Ŭ���̾�Ʈ �� ������ ���� ������

int main(int argc, char *argv[])
{
	LoadUserInfo("userDatabase.txt");		// ���� ���� �ҷ�����

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;
	InitializeCriticalSection(&cs);
	// ���� �ʱ�ȭ
	SOCKET listen_sock = sock_init();

	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	int size;

	// 0��°�� �̺�Ʈ �ֱ�, ThreadCount �����ϱ� ����
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

// ���� �Լ� ���� ���
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
		printf("���� ���� ����\n");
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

	printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
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

			printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
				inet_ntoa(_ptr->addr.sin_addr), ntohs(_ptr->addr.sin_port));

			break;
		}
	}

	LeaveCriticalSection(&cs);
}

// ��ŷ �Լ� ����
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


// ����ŷ �Լ� ����
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
		case INIT_STATE:						// �ʱ�ȭ ����
			ptr->state = MAINMENU_STATE;		// ���θ޴� ���·� �̵�
			break;
		case MAINMENU_STATE:					// ���θ޴� ����
			if (!PacketRecv(ptr->sock, ptr->buf))
			{
				endflag = true;
				break;
			}

			protocol = GetProtocol(ptr->buf);

			UnPack_userInfo(ptr->buf, &user);	// �Է¹��� ������ ������ �޾ƿ´�
			LOGIN_CODE login_code;
			REG_CODE reg_code;
			int check;

			switch (protocol)
			{
			case LOGIN:								// �α����̶��?
				check = SearchLogin(ptr, user);		// �α����� ��쿡 ���缭 ��� �Ǻ����ش�
													// ����� �´� �޽����� �������ش�
				protocol = LOGIN;

				switch (check)
				{
				case LOGIN_CODE::WORNG_ID:				// ���̵� Ʋ��
					strcpy(msg, MSG_WORNG_ID);
					login_code = LOGIN_CODE::WORNG_ID;
					break;
				case LOGIN_CODE::WORNG_PW:				// ��й�ȣ Ʋ��
					strcpy(msg, MSG_WRONG_PW);
					login_code = LOGIN_CODE::WORNG_PW;
					break;
				case LOGIN_CODE::INUSE:					// �̹� �α�����
					strcpy(msg, MSG_INUSE);
					login_code = LOGIN_CODE::INUSE;
					break;
				case LOGIN_CODE::SUCCESS:				// �α��� ����
					strcpy(msg, MSG_SUCCESS);
					login_code = LOGIN_CODE::SUCCESS;
					ptr->state = LOGINMENU_STATE;		// �α��� ���������� �α��θ޴� ���·� ����
					break;
				}
				size = Pack_mainMenu(ptr->buf, protocol, login_code, msg);

				break;
			case REGISTER:							// ȸ�������� ��쵵 �α��ΰ� ������
				check = SearchReg(user);
				protocol = REGISTER;

				switch (check)
				{
				case REG_CODE::EXIST_ID:			// �̹� �����ϴ� ���̵�
					strcpy(msg, MSG_EXIST_ID);
					reg_code = REG_CODE::EXIST_ID;
					break;
				case REG_CODE::ENABLED:				// ȸ������ ����
					AddUserInfo(user);				// �Է¹��� ������ ȸ������

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
		case LOGINMENU_STATE:						// �α��ο� ������ ����
			while (1)
			{
				bool escapeflag = false;
				char buf[BUFSIZE];
				bool fileflag = false;
				bool sendfail = false;
				bool file_escapeflag = false;
				FILE* fp = nullptr;

				if (!PacketRecv(ptr->sock, ptr->buf))	// �α��θ޴����� ������ �޴��� �޴´�
														// ��������, �α׾ƿ�, ȸ��Ż��
				{
					endflag = true;
					break;
				}

				protocol = GetProtocol(ptr->buf);

				switch (protocol)
				{
				case FILE_UPLOAD:						// ���� ����
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
						case FILE_INFO:						// ���� ���� �ޱ�
							char filename[FILENAMESIZE];
							int filesize;
							memset(filename, 0, sizeof(filename));

							UnPack_fileinfo(ptr->buf, filename, filesize);		// ���� �̸��� ũ�⸦ ����ŷ

							strcpy(ptr->file_info.filename, filename);			// �̸��� �������ش�
							// ���� ������ ���ε� ���� Ŭ���̾�Ʈ�� ������, ��ٸ���.
							for (int i = 0; i < clientCount; i++)
							{
								if (ptr != ClientInfo[i])
								{
									// �ٸ� Ŭ���̾�Ʈ�� �����Ϸ��� ���� �̸��� ����, �������̸�?
									if (strcmp(ptr->file_info.filename, ClientInfo[i]->file_info.filename) == 0
										&& ClientInfo[i]->isUploading)
									{
										// �̺�Ʈ�� ���ֱ� ��ٸ���
										WaitForSingleObject(ptr->wait_upload_event, INFINITE);
										break; // �Ѹ� ó���ǵ��� �극��ũ
									}
								}
							}

							if (strlen(filename) > MAX_NAMESIZE)	// ���� �̸��� ���ѵǴ°�?
							{										// ���� �ź� �ڵ� ������
								int size = Pack_transDeny(ptr->buf, FILE_TRANS_DENY, NAMESIZE_DENY, FILE_NAMESIZE_DENY_MSG);
								sendfail = true;
								file_escapeflag = true;

								retval = send(ptr->sock, ptr->buf, size, 0);
								if (retval == SOCKET_ERROR)
								{
									err_display("file trans deny send()");
								}

								printf("���� �ź� - ���� �̸� ũ�� ����\n");

								break;
							}
							else if (filesize > MAX_FILESIZE)	// ���� ũ�Ⱑ ���ѵǴ°�?
							{									// ���� �ź� �ڵ� ������
								int size = Pack_transDeny(ptr->buf, FILE_TRANS_DENY, SIZE_DENY, FILE_SIZE_DENY_MSG);
								sendfail = true;
								file_escapeflag = true;

								retval = send(ptr->sock, ptr->buf, size, 0);
								if (retval == SOCKET_ERROR)
								{
									err_display("file trans deny send()");
								}

								printf("���� �ź� - ���� ũ�� ����\n");

								break;
							}
							else //	���� �̸��� ũ��� ���� ����
							{
								if (SearchFile(filename))		// �̹� �����ϴ� �����ΰ�?
								{
									fp = fopen(filename, "rb");
									fseek(fp, 0, SEEK_END);
									int fsize = ftell(fp);		// ���� ���� ������ ũ��
									fclose(fp);
									if (filesize == fsize)		// �������� ���� ũ�Ⱑ ���� ���� ���ϰ� ���ٸ�?
																// �̹� �ִ� �����̹Ƿ� ���� �ź�
									{
										printf("���� �ź� - �����ϴ� ���� ���� �䱸\n");

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
									else // ����ũ�Ⱑ �ٸ� ��쿣 �̾ �޵��� �Ѵ�
									{
										strcpy(ptr->file_info.filename, filename);
										ptr->file_info.filesize = filesize;
										ptr->file_info.nowsize = fsize;
									}

								}
								else //	�װ͵� �ƴϸ� ó������ �޴´�
								{
									strcpy(ptr->file_info.filename, filename);
									ptr->file_info.filesize = filesize;
									ptr->file_info.nowsize = 0;
								}

								// ������ �������� ���� ���� ���� ������
								size = Pack_transStartPoint(ptr->buf, FILE_TRANS_START_POINT, ptr->file_info.nowsize);

								retval = send(ptr->sock, ptr->buf, size, 0);
								if (retval == SOCKET_ERROR)
								{
									err_display("file trans start point send()");
									break;
								}
							}
							break;
						case FILE_TRANS_DATA:	// ���� �ޱ�
							if (!fileflag)		// ó������ fileflag�� false�� (�ѹ��� ���� ����)
							{
								fileflag = true;							// true�� �ٲ��ְ�
								fp = fopen(ptr->file_info.filename, "ab");	// ������ �����ش� 
								ptr->isUploading = true;					// �̶� ���� ���ε� ���̶�°� ǥ���ϱ� ����
																			// isUploading�� true�� �ٲ��ش�
							}

							// �뷫 ���� �޴� �κ�..
							int transsize;
							UnPack_transData(ptr->buf, transsize, buf);
							fwrite(buf, 1, transsize, fp);
							if (ferror(fp)) {
								perror("���� ����� ����");
								file_escapeflag = true;
								break;
							}
							ptr->file_info.nowsize += transsize;	// ���� ũ�⿡ ���� ũ�⸸ŭ ��� �����ش�
							break;
						}

						// ����ũ�Ⱑ 0�� �ƴϰ�, �������ߴ� ���� ũ��� ���� ���� ũ�Ⱑ ���ٸ�?
						// ���� �Ϸ�� ����
						if (ptr->file_info.filesize != 0 && ptr->file_info.filesize == ptr->file_info.nowsize)
						{
							printf("���ۿϷ�\n");
							fclose(fp);

							file_escapeflag = true;
							ptr->isUploading = false;

							// ������ ���� ��, ��ٸ��� Ŭ���̾�Ʈ�� �̺�Ʈ�� ���ش�
							for (int i = 0; i < clientCount; i++)
							{
								if (ptr != ClientInfo[i])
								{
									// ���� ������ ���� �̸��� ��밡 �����Ϸ��� ���� �̸��� ����,
									// ���� ���ε����� �ƴ϶��?
									if (strcmp(ptr->file_info.filename, ClientInfo[i]->file_info.filename) == 0
										&& !ClientInfo[i]->isUploading)
									{
										// �̺�Ʈ�� ���ش�
										SetEvent(ClientInfo[i]->wait_upload_event);
										break;	// �Ѹ� ���ֱ� ���� �극��ũ
									}
								}
							}

							ZeroMemory(&ptr->file_info, sizeof(_File_info));
						}
						else if (sendfail)
						{
							sendfail = false;
							printf("���۽���\n");
						}

						if (file_escapeflag)
							break;
					}//end while
					break;
				case LOGOUT:					// �α׾ƿ�
					escapeflag = true;
					ptr->user->isLogin = false;	// �α׾ƿ��̴� false�� �ٲ۴�
					ptr->user = nullptr;
					break;
				case DELETEACC:					// ȸ��Ż��
					escapeflag = true;
					ptr->user->isLogin = false;
					RemoveUserInfo(ptr->user);	// ȸ�� ���� ��� ����
					break;
				}

				if (escapeflag)
				{
					ptr->state = MAINMENU_STATE;	// ���θ޴��� ���ư���
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

DWORD WINAPI RemoveClient(LPVOID _ptr)			// Ŭ���̾�Ʈ �� ������ ����
{
	while (1)
	{
		int index = WaitForMultipleObjects(ThreadCount, hThread, false, INFINITE);	// ������ ���Ḧ ��ٸ���
		index -= WAIT_OBJECT_0;
		EnterCriticalSection(&cs);
		if (index == 0)
		{
			LeaveCriticalSection(&cs);
			continue;
		}

		_ClientInfo* ptr = SearchClientInfo(hThread[index]);		// �����尡 ����ϴ� Ŭ���̾�Ʈ�� ã�´�
		if (ptr == nullptr)					// Ŭ���̾�Ʈ�� ������?
		{
			RemoveThread(hThread[index]);	// �ٷ� ���� ��
			LeaveCriticalSection(&cs);		// Ű �ݳ�
			continue;
		}

		RemoveThread(hThread[index]);		// ������ ���� ����

		ptr->hthread = nullptr;

		switch (ptr->state)					// Ŭ���̾�Ʈ�� ������ ���¿� ���� ���� ����
		{
		case INIT_STATE:
		case MAINMENU_STATE:
		case LOGINMENU_STATE:				// üũ�ؾ� �ϴ°� ���ε��������� üũ�ϸ� �Ǳ� ������
											// ���ε� ���� üũ���ش�
			if (ptr->isUploading)			// ���������� ���ε� ���̾��ٸ�?
											// ��� ���̴� Ŭ���̾�Ʈ�� ����� �Ѵ�
			{
				for (int i = 0; i < clientCount; i++)
				{
					if (ptr != ClientInfo[i])	// ���� �����ϰ�
					{
						// ���� �̸��� �����ϰ� ���ε������� ���� Ŭ���̾�Ʈ�� ã����? (��ٸ��� �ִ� Ŭ���̾�Ʈ)
						if (strcmp(ptr->file_info.filename, ClientInfo[i]->file_info.filename) == 0
							&& !ClientInfo[i]->isUploading)
						{
							// �̺�Ʈ�� ���ش�
							SetEvent(ClientInfo[i]->wait_upload_event);
							break;	// �Ѹ� ��� Ǯ������ �극��ũ
						}
					}
				}
			}
			if(ptr->user != nullptr)		// ���� ������ �ִٸ�?
				ptr->user->isLogin = false;	// �α׾ƿ� �����ش�. (�ٸ� Ŭ���̾�Ʈ�� �ش� ���̵�� �α����� �� �ֵ���)
			RemoveClientInfo(ptr);			// �۾��Ϸ��� Ŭ���̾�Ʈ ���� ����
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