#pragma warning(disable : 4996)
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE    4096
#define SIZE 10
#define FILENAMESIZE 256
#define MAX_FILE_READ_SIZE 2048

enum PROTOCOL		// 메뉴를 나누기 위한 프로토콜
{
	MAINMENU = 1,	// 메인메뉴
	LOGIN,			// 로그인
	REGISTER,		// 회원가입
	EXIT,			// 종료
	LOGINMENU,		// 로그인메뉴
	FILE_UPLOAD,	// 파일전송
	LOGOUT,			// 로그아웃
	DELETEACC,		// 회원탈퇴
};

enum LOGIN_CODE		// 로그인 결과 코드
{
	WORNG_ID = 1,	//	아이디 다름
	WORNG_PW,		//	비밀번호 다름
	INUSE,			//	이미 로그인 중
	SUCCESS			//	로그인 성공
};

enum REG_CODE		// 회원가입 결과 코드
{
	EXIST_ID = 1,	//	존재하는 아이디
	ENABLED,		//	생성가능
};

enum FILE_CODE		// 파일 전송시 사용되는 프로토콜
{
	FILE_INTRO=-1,			// 파일 선택을 위한 인트로 (전송가능한 파일 나열해줌)
	FILE_INFO=1,			// 파일의 정보
	FILE_TRANS_DENY,		// 파일 전송 거부
	FILE_TRANS_START_POINT,	// 파일 전송 시작지점
	FILE_TRANS_DATA,		// 파일 전송
};

enum DENY_CODE		// 파일 전송 거부 코드
{
	FILEEXIST = -1,		// 파일이 이미 존재
	NAMESIZE_DENY = 1,	// 이름 크기 제한
	SIZE_DENY			// 파일 크기 제한
};

typedef struct user		// 유저 정보
{
	char id[SIZE];
	char pw[SIZE];
	bool isLogin;
}_UserInfo;

struct _File_info		// 파일 정보
{
	char filename[FILENAMESIZE];
	int  filesize;
	int  nowsize;
}Fileinfo;

void err_quit(const char *msg);
void err_display(const char *msg);
int recvn(SOCKET s, char *buf, int len, int flags);
SOCKET sock_init();

bool PacketRecv(SOCKET _sock, char* _buf);
PROTOCOL GetProtocol(const char* _ptr);
FILE_CODE GetFilecode(const char* _ptr);

// 팩킹
int Pack_userInfo(char* _buf, PROTOCOL _protocol, _UserInfo user);
int Pack_protocol(char* _buf, PROTOCOL _protocol);
int Pack_fileInfo(char* _buf, FILE_CODE _fprotocol, const char* str, int data);
int Pack_transData(char* _buf, FILE_CODE _protocol, int _datasize, const char* _filedata);

// 언팩킹
void UnPack_mainMenu(const char* _buf, LOGIN_CODE& code, char* msg);
void UnPack_mainMenu(const char* _buf, REG_CODE& code, char* msg);
void UnPack_denyCode(const char* _buf, int& _data, char* _str);
void UnPack_transStartPoint(const char* _buf, int& _data);

int main(int argc, char *argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET sock = sock_init();

	PROTOCOL protocol = MAINMENU;

	_UserInfo user;
	char buf[BUFSIZE];
	ZeroMemory(buf, sizeof(buf));
	char msg[BUFSIZE];
	bool endflag = false;
	int size;


	while (1)
	{
		switch (protocol)
		{
		case MAINMENU:					// 메인메뉴
			system("cls");
			int select;

			printf("1. 로그인\n");		// 메뉴 출력
			printf("2. 회원가입\n");
			printf("3. 종료\n");
			printf("선택: ");
			scanf("%d", &select);

			switch (select)				// 선택한 메뉴에 따라 프로토콜 설정
			{
			case 1:
				protocol = LOGIN;
				break;
			case 2:
				protocol = REGISTER;
				break;
			case 3:
				exit(0);
				break;
			}

			system("cls");

			printf("ID: ");			// 아이디와 패스워드를 입력받아서
			scanf("%s", user.id);
			printf("PW: ");
			scanf("%s", user.pw);

			size = Pack_userInfo(buf, protocol, user);	// 패킹한 뒤에

			retval = send(sock, buf, size, 0);			// 서버에게 보낸다
			if (retval == SOCKET_ERROR)
			{
				err_display("userinfo send()");
				break;
			}

			if (!PacketRecv(sock, buf))		// 서버로부터 답신을 받고
			{
				break;
			}

			protocol = GetProtocol(buf);	// 프로토콜을 나눈 뒤

			switch (protocol)
			{
			case LOGIN:						// 프로토콜에 맞는 메시지와 결과 코드를 받는다
				LOGIN_CODE login_code;

				ZeroMemory(msg, sizeof(msg));
				UnPack_mainMenu(buf, login_code, msg);

				printf("%s\n", msg);

				switch (login_code)
				{
				case LOGIN_CODE::WORNG_ID:	// 틀린 아이디
				case LOGIN_CODE::WORNG_PW:	// 틀린 패스워드
				case LOGIN_CODE::INUSE:		// 이미 로그인중인 아이디
					protocol = MAINMENU;	// 다시 메인메뉴로 돌아간다
					break;
				case LOGIN_CODE::SUCCESS:	// 로그인 성공
					protocol = LOGINMENU;	// 로그인메뉴로 이동
					break;
				}

				system("pause");
				break;
			case REGISTER:
				REG_CODE reg_code;

				ZeroMemory(msg, sizeof(msg));
				UnPack_mainMenu(buf, reg_code, msg);

				printf("%s\n", msg);

				switch (reg_code)
				{
				case REG_CODE::EXIST_ID:	// 이미 존재하는 아이디
				case REG_CODE::ENABLED:		// 사용가능, 회원가입 성공
					protocol = MAINMENU;	// 다시 메인메뉴로
					break;
				}

				system("pause");
				break;
			}

			break;
		case LOGINMENU:						// 로그인메뉴
			while (1)
			{
				system("cls");

				int select;
				bool escapeflag = false;

				printf("1. 파일전송\n");	// 메뉴를 출력한다
				printf("2. 로그아웃\n");
				printf("3. 회원탈퇴\n");
				printf("선택: ");
				scanf("%d", &select);

				FILE_CODE fcode = FILE_INTRO;	// 처음엔 인트로 셋팅
				bool file_escapeflag = false;
				FILE* fp = nullptr;

				switch (select+5)
				{
				case FILE_UPLOAD:		// 파일전송
					size = Pack_protocol(buf, FILE_UPLOAD);		// 서버에게 파일 전송을 선택했다는 패킷전송

					retval = send(sock, buf, size, 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("loginmenu protocol send()");
						break;
					}

					while (1)
					{
						switch (fcode)
						{
						case FILE_INTRO:		// 처음엔 무조건 여기로 들어온다
							int count;

							while (true)
							{
								system("cls");

								printf("----파일목록----\n");		// 전송 가능한 파일 목록을 보여준다

								for (int i = 1; i < argc; i++)
								{
									printf("%d: %s\n", i, argv[i]);
								}
								printf("선택: ");
								scanf("%d", &count);

								if (0 < count && count < argc)
									break;
								else
								{
									printf("잘못 입력하셨습니다\n");
									system("pause");
									system("cls");
								}
							}

							ZeroMemory(&Fileinfo, sizeof(Fileinfo));
							strcpy(Fileinfo.filename, argv[count]);		// 선택한 파일 이름을 저장하고

							fp = fopen(Fileinfo.filename, "rb");		// 파일을 열어서
							if (!fp)
							{
								err_quit("fopen");
							}

							fseek(fp, 0, SEEK_END);
							Fileinfo.filesize = ftell(fp);				// 파일의 크기를 잰 뒤
							fclose(fp);									// 닫아준다

							// 서버에게 전송하려는 파일 이름과 크기 정보를 보내준다
							size = Pack_fileInfo(buf, FILE_INFO, Fileinfo.filename, Fileinfo.filesize);

							retval = send(sock, buf, size, 0);
							if (retval == SOCKET_ERROR)
							{
								err_quit("file info send()");
							}

							if (!PacketRecv(sock, buf))		// 서버의 답신을 기다린다
							{
								break;
							}

							fcode = GetFilecode(buf);

							break;
						case FILE_TRANS_DENY:				// 전송 불가하다는 답신을 받은 경우
							int deny_code;
							memset(msg, 0, sizeof(msg));
							UnPack_denyCode(buf, deny_code, msg);
							switch (deny_code)
							{
							case FILEEXIST:					// 이미 존재하는 파일
							case NAMESIZE_DENY:				// 이름 크기가 제한된다
							case SIZE_DENY:					// 파일 크기가 제한된다
								printf("%s\n", msg);		// 메시지 출력
								file_escapeflag = true;		// 다시 로그인 메뉴로
								break;
							}

							system("pause");
							break;
						case FILE_TRANS_START_POINT:		// 전송 가능하니 어디서부터 보낼지?
							UnPack_transStartPoint(buf, Fileinfo.nowsize);	// 서버가 가진 파일의 크기를 받아서

							fp = fopen(Fileinfo.filename, "rb");
							fseek(fp, Fileinfo.nowsize, SEEK_SET);	// 파일의 시작지점을 세팅한다

							while (1)
							{
								char rbuf[BUFSIZE];
								int trans_size = fread(rbuf, 1, MAX_FILE_READ_SIZE, fp);	// MAX_SIZE_READ_SIZE만큼 읽어서
								if (trans_size > 0)	// 읽은 크기가 0이 아니면
								{
									// 읽은 크기만큼 서버에게 보낸다
									int size = Pack_transData(buf, FILE_TRANS_DATA, trans_size, rbuf);
									retval = send(sock, buf, size, 0);
									if (retval == SOCKET_ERROR)
									{
										err_display("send()");
										break;
									}
									Fileinfo.nowsize += trans_size;	// 현재크기에는 계속해서 더해준다
									printf("..");
									Sleep(2000);	// 딜레이타임 2초로 설정
								}
								// 더이상 읽을 파일이 없고, 현재 크기가 파일 전체 크기와 같다면 전송 완료한것
								else if (trans_size == 0 && Fileinfo.nowsize == Fileinfo.filesize)
								{
									printf("파일 전송 완료!: %d 바이트\n", Fileinfo.nowsize);
									system("pause");
									break;
								}
								else
								{
									perror("파일 입출력 오류");
									break;
								}
							}

							fclose(fp);

							file_escapeflag = true;
							break;

						}// end fcode switch

						if (file_escapeflag)
							break;
					}// end while
					break;
				case LOGOUT:		// 로그아웃
					escapeflag = true;

					//로그아웃 프로토콜 전송
					size = Pack_protocol(buf, LOGOUT);

					retval = send(sock, buf, size, 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("loginmenu protocol send()");
						break;
					}
					break;
				case DELETEACC:		// 회원탈퇴
					escapeflag = true;

					//회원탈퇴 프로토콜 전송
					size = Pack_protocol(buf, DELETEACC);

					retval = send(sock, buf, size, 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("loginmenu protocol send()");
						break;
					}
					break;
				default:
					printf("다시 입력하세요.\n");
					system("pause");
					break;
				}

				if (escapeflag)
				{
					protocol = MAINMENU;
					break;
				}
			}

			break;
		}

		if (endflag)
			break;
	}
	closesocket(sock);

	WSACleanup();
	system("pause");
	return 0;
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
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);
	int retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");

	return sock;
}

int Pack_userInfo(char* _buf, PROTOCOL _protocol, _UserInfo user)
{
	int size = 0;
	char* ptr = _buf;
	int strsize = strlen(user.id);

	ptr = ptr + sizeof(size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	size = size + sizeof(_protocol);

	memcpy(ptr, &strsize, sizeof(strsize));
	ptr = ptr + sizeof(strsize);
	size = size + sizeof(strsize);

	memcpy(ptr, user.id, strsize);
	ptr = ptr + strsize;
	size = size + strsize;

	strsize = strlen(user.pw);
	memcpy(ptr, &strsize, sizeof(strsize));
	ptr = ptr + sizeof(strsize);
	size = size + sizeof(strsize);

	memcpy(ptr, user.pw, strsize);
	ptr = ptr + strsize;
	size = size + strsize;

	ptr = _buf;
	memcpy(ptr, &size, sizeof(size));

	size = size + sizeof(size);

	return size;
}

int Pack_protocol(char* _buf, PROTOCOL _protocol)
{
	int size = 0;
	char* ptr = _buf;

	ptr = ptr + sizeof(size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	size = size + sizeof(_protocol);

	ptr = _buf;
	memcpy(ptr, &size, sizeof(size));

	size = size + sizeof(size);

	return size;
}

int Pack_fileInfo(char* _buf, FILE_CODE _protocol, const char* _str1, int _data)
{
	char* ptr = _buf;
	int strsize = strlen(_str1);
	int size = 0;

	ptr = ptr + sizeof(size);

	memcpy(ptr, &_protocol, sizeof(FILE_CODE));
	ptr = ptr + sizeof(FILE_CODE);
	size = size + sizeof(FILE_CODE);

	memcpy(ptr, &strsize, sizeof(strsize));
	ptr = ptr + sizeof(strsize);
	size = size + sizeof(strsize);

	memcpy(ptr, _str1, strsize);
	ptr = ptr + strsize;
	size = size + strsize;

	memcpy(ptr, &_data, sizeof(_data));
	ptr = ptr + sizeof(_data);
	size = size + sizeof(_data);

	ptr = _buf;

	memcpy(ptr, &size, sizeof(size));

	size = size + sizeof(size);

	return size;
}

int Pack_transData(char* _buf, FILE_CODE _protocol, int _datasize, const char* _filedata)
{
	char* ptr = _buf;
	int size = 0;

	ptr = ptr + sizeof(size);

	memcpy(ptr, &_protocol, sizeof(FILE_CODE));
	ptr = ptr + sizeof(FILE_CODE);
	size = size + sizeof(FILE_CODE);

	memcpy(ptr, &_datasize, sizeof(_datasize));
	ptr = ptr + sizeof(_datasize);
	size = size + sizeof(_datasize);

	memcpy(ptr, _filedata, _datasize);
	ptr = ptr + _datasize;
	size = size + _datasize;

	ptr = _buf;

	memcpy(ptr, &size, sizeof(size));

	size = size + sizeof(size);

	return size;
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

void UnPack_mainMenu(const char* _buf, LOGIN_CODE& code, char* msg)
{
	const char* ptr = _buf + sizeof(PROTOCOL);
	int strsize;

	memcpy(&code, ptr, sizeof(LOGIN_CODE));
	ptr = ptr + sizeof(LOGIN_CODE);

	memcpy(&strsize, ptr, sizeof(int));
	ptr = ptr + sizeof(int);

	memcpy(msg, ptr, strsize);
}

void UnPack_mainMenu(const char* _buf, REG_CODE& code, char* msg)
{
	const char* ptr = _buf + sizeof(PROTOCOL);
	int strsize;

	memcpy(&code, ptr, sizeof(REG_CODE));
	ptr = ptr + sizeof(REG_CODE);

	memcpy(&strsize, ptr, sizeof(int));
	ptr = ptr + sizeof(int);

	memcpy(msg, ptr, strsize);
}

void UnPack_denyCode(const char* _buf, int& _data, char* _str)
{
	const char* ptr = _buf;
	int strsize;

	ptr = ptr + sizeof(PROTOCOL);

	memcpy(&_data, ptr, sizeof(_data));
	ptr = ptr + sizeof(_data);

	memcpy(&strsize, ptr, sizeof(strsize));
	ptr = ptr + sizeof(strsize);

	memcpy(_str, ptr, strsize);
}

void UnPack_transStartPoint(const char* _buf, int& _data)
{
	const char* ptr = _buf + sizeof(PROTOCOL);

	memcpy(&_data, ptr, sizeof(_data));
}