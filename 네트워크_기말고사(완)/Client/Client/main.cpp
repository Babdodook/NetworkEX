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

enum PROTOCOL		// �޴��� ������ ���� ��������
{
	MAINMENU = 1,	// ���θ޴�
	LOGIN,			// �α���
	REGISTER,		// ȸ������
	EXIT,			// ����
	LOGINMENU,		// �α��θ޴�
	FILE_UPLOAD,	// ��������
	LOGOUT,			// �α׾ƿ�
	DELETEACC,		// ȸ��Ż��
};

enum LOGIN_CODE		// �α��� ��� �ڵ�
{
	WORNG_ID = 1,	//	���̵� �ٸ�
	WORNG_PW,		//	��й�ȣ �ٸ�
	INUSE,			//	�̹� �α��� ��
	SUCCESS			//	�α��� ����
};

enum REG_CODE		// ȸ������ ��� �ڵ�
{
	EXIST_ID = 1,	//	�����ϴ� ���̵�
	ENABLED,		//	��������
};

enum FILE_CODE		// ���� ���۽� ���Ǵ� ��������
{
	FILE_INTRO=-1,			// ���� ������ ���� ��Ʈ�� (���۰����� ���� ��������)
	FILE_INFO=1,			// ������ ����
	FILE_TRANS_DENY,		// ���� ���� �ź�
	FILE_TRANS_START_POINT,	// ���� ���� ��������
	FILE_TRANS_DATA,		// ���� ����
};

enum DENY_CODE		// ���� ���� �ź� �ڵ�
{
	FILEEXIST = -1,		// ������ �̹� ����
	NAMESIZE_DENY = 1,	// �̸� ũ�� ����
	SIZE_DENY			// ���� ũ�� ����
};

typedef struct user		// ���� ����
{
	char id[SIZE];
	char pw[SIZE];
	bool isLogin;
}_UserInfo;

struct _File_info		// ���� ����
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

// ��ŷ
int Pack_userInfo(char* _buf, PROTOCOL _protocol, _UserInfo user);
int Pack_protocol(char* _buf, PROTOCOL _protocol);
int Pack_fileInfo(char* _buf, FILE_CODE _fprotocol, const char* str, int data);
int Pack_transData(char* _buf, FILE_CODE _protocol, int _datasize, const char* _filedata);

// ����ŷ
void UnPack_mainMenu(const char* _buf, LOGIN_CODE& code, char* msg);
void UnPack_mainMenu(const char* _buf, REG_CODE& code, char* msg);
void UnPack_denyCode(const char* _buf, int& _data, char* _str);
void UnPack_transStartPoint(const char* _buf, int& _data);

int main(int argc, char *argv[])
{
	int retval;

	// ���� �ʱ�ȭ
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
		case MAINMENU:					// ���θ޴�
			system("cls");
			int select;

			printf("1. �α���\n");		// �޴� ���
			printf("2. ȸ������\n");
			printf("3. ����\n");
			printf("����: ");
			scanf("%d", &select);

			switch (select)				// ������ �޴��� ���� �������� ����
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

			printf("ID: ");			// ���̵�� �н����带 �Է¹޾Ƽ�
			scanf("%s", user.id);
			printf("PW: ");
			scanf("%s", user.pw);

			size = Pack_userInfo(buf, protocol, user);	// ��ŷ�� �ڿ�

			retval = send(sock, buf, size, 0);			// �������� ������
			if (retval == SOCKET_ERROR)
			{
				err_display("userinfo send()");
				break;
			}

			if (!PacketRecv(sock, buf))		// �����κ��� ����� �ް�
			{
				break;
			}

			protocol = GetProtocol(buf);	// ���������� ���� ��

			switch (protocol)
			{
			case LOGIN:						// �������ݿ� �´� �޽����� ��� �ڵ带 �޴´�
				LOGIN_CODE login_code;

				ZeroMemory(msg, sizeof(msg));
				UnPack_mainMenu(buf, login_code, msg);

				printf("%s\n", msg);

				switch (login_code)
				{
				case LOGIN_CODE::WORNG_ID:	// Ʋ�� ���̵�
				case LOGIN_CODE::WORNG_PW:	// Ʋ�� �н�����
				case LOGIN_CODE::INUSE:		// �̹� �α������� ���̵�
					protocol = MAINMENU;	// �ٽ� ���θ޴��� ���ư���
					break;
				case LOGIN_CODE::SUCCESS:	// �α��� ����
					protocol = LOGINMENU;	// �α��θ޴��� �̵�
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
				case REG_CODE::EXIST_ID:	// �̹� �����ϴ� ���̵�
				case REG_CODE::ENABLED:		// ��밡��, ȸ������ ����
					protocol = MAINMENU;	// �ٽ� ���θ޴���
					break;
				}

				system("pause");
				break;
			}

			break;
		case LOGINMENU:						// �α��θ޴�
			while (1)
			{
				system("cls");

				int select;
				bool escapeflag = false;

				printf("1. ��������\n");	// �޴��� ����Ѵ�
				printf("2. �α׾ƿ�\n");
				printf("3. ȸ��Ż��\n");
				printf("����: ");
				scanf("%d", &select);

				FILE_CODE fcode = FILE_INTRO;	// ó���� ��Ʈ�� ����
				bool file_escapeflag = false;
				FILE* fp = nullptr;

				switch (select+5)
				{
				case FILE_UPLOAD:		// ��������
					size = Pack_protocol(buf, FILE_UPLOAD);		// �������� ���� ������ �����ߴٴ� ��Ŷ����

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
						case FILE_INTRO:		// ó���� ������ ����� ���´�
							int count;

							while (true)
							{
								system("cls");

								printf("----���ϸ��----\n");		// ���� ������ ���� ����� �����ش�

								for (int i = 1; i < argc; i++)
								{
									printf("%d: %s\n", i, argv[i]);
								}
								printf("����: ");
								scanf("%d", &count);

								if (0 < count && count < argc)
									break;
								else
								{
									printf("�߸� �Է��ϼ̽��ϴ�\n");
									system("pause");
									system("cls");
								}
							}

							ZeroMemory(&Fileinfo, sizeof(Fileinfo));
							strcpy(Fileinfo.filename, argv[count]);		// ������ ���� �̸��� �����ϰ�

							fp = fopen(Fileinfo.filename, "rb");		// ������ ���
							if (!fp)
							{
								err_quit("fopen");
							}

							fseek(fp, 0, SEEK_END);
							Fileinfo.filesize = ftell(fp);				// ������ ũ�⸦ �� ��
							fclose(fp);									// �ݾ��ش�

							// �������� �����Ϸ��� ���� �̸��� ũ�� ������ �����ش�
							size = Pack_fileInfo(buf, FILE_INFO, Fileinfo.filename, Fileinfo.filesize);

							retval = send(sock, buf, size, 0);
							if (retval == SOCKET_ERROR)
							{
								err_quit("file info send()");
							}

							if (!PacketRecv(sock, buf))		// ������ ����� ��ٸ���
							{
								break;
							}

							fcode = GetFilecode(buf);

							break;
						case FILE_TRANS_DENY:				// ���� �Ұ��ϴٴ� ����� ���� ���
							int deny_code;
							memset(msg, 0, sizeof(msg));
							UnPack_denyCode(buf, deny_code, msg);
							switch (deny_code)
							{
							case FILEEXIST:					// �̹� �����ϴ� ����
							case NAMESIZE_DENY:				// �̸� ũ�Ⱑ ���ѵȴ�
							case SIZE_DENY:					// ���� ũ�Ⱑ ���ѵȴ�
								printf("%s\n", msg);		// �޽��� ���
								file_escapeflag = true;		// �ٽ� �α��� �޴���
								break;
							}

							system("pause");
							break;
						case FILE_TRANS_START_POINT:		// ���� �����ϴ� ��𼭺��� ������?
							UnPack_transStartPoint(buf, Fileinfo.nowsize);	// ������ ���� ������ ũ�⸦ �޾Ƽ�

							fp = fopen(Fileinfo.filename, "rb");
							fseek(fp, Fileinfo.nowsize, SEEK_SET);	// ������ ���������� �����Ѵ�

							while (1)
							{
								char rbuf[BUFSIZE];
								int trans_size = fread(rbuf, 1, MAX_FILE_READ_SIZE, fp);	// MAX_SIZE_READ_SIZE��ŭ �о
								if (trans_size > 0)	// ���� ũ�Ⱑ 0�� �ƴϸ�
								{
									// ���� ũ�⸸ŭ �������� ������
									int size = Pack_transData(buf, FILE_TRANS_DATA, trans_size, rbuf);
									retval = send(sock, buf, size, 0);
									if (retval == SOCKET_ERROR)
									{
										err_display("send()");
										break;
									}
									Fileinfo.nowsize += trans_size;	// ����ũ�⿡�� ����ؼ� �����ش�
									printf("..");
									Sleep(2000);	// ������Ÿ�� 2�ʷ� ����
								}
								// ���̻� ���� ������ ����, ���� ũ�Ⱑ ���� ��ü ũ��� ���ٸ� ���� �Ϸ��Ѱ�
								else if (trans_size == 0 && Fileinfo.nowsize == Fileinfo.filesize)
								{
									printf("���� ���� �Ϸ�!: %d ����Ʈ\n", Fileinfo.nowsize);
									system("pause");
									break;
								}
								else
								{
									perror("���� ����� ����");
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
				case LOGOUT:		// �α׾ƿ�
					escapeflag = true;

					//�α׾ƿ� �������� ����
					size = Pack_protocol(buf, LOGOUT);

					retval = send(sock, buf, size, 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("loginmenu protocol send()");
						break;
					}
					break;
				case DELETEACC:		// ȸ��Ż��
					escapeflag = true;

					//ȸ��Ż�� �������� ����
					size = Pack_protocol(buf, DELETEACC);

					retval = send(sock, buf, size, 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("loginmenu protocol send()");
						break;
					}
					break;
				default:
					printf("�ٽ� �Է��ϼ���.\n");
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