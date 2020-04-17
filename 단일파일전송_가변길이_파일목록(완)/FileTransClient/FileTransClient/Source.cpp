#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define BUFSIZE 4096
#define MAX_FILE_READ_SIZE 2048
#define FILENAMESIZE 256

enum PROTOCOL
{
	INTRO = 1,
	FILE_INFO,
	FILE_TRANS_DENY,
	FILE_TRANS_START_POINT,
	FILE_TRANS_DATA,
	REQUEST
};

enum DENY_CODE
{
	FILEEXIST = -1
};

enum REQ_CODE 
{
	CONTINUE=1,
	END
};

struct _File_info
{
	char filename[FILENAMESIZE];
	int  filesize;
	int  nowsize;
}Fileinfo;


void err_quit(const char *msg);
void err_display(const char *msg);
int recvn(SOCKET s, char *buf, int len, int flags);

bool PacketRecv(SOCKET, char*);

int PackPacket(char* _buf, PROTOCOL _protocol, const char* _str1, int _data);
int PackPacket(char*, PROTOCOL, int, const char*);
int PackPacket(char*, PROTOCOL, int);

PROTOCOL GetProtocol(const char*);

void UnPackPacket(const char*, int&, char*);
void UnPackPacket(const char*, char*);
void UnPackPacket(const char*, int&);

int main(int argc, char* argv[])
{
	int retval;
	char buf[BUFSIZE];
	
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");

	ZeroMemory(&Fileinfo, sizeof(Fileinfo));
		
	FILE* fp = nullptr;	
	bool endflag = false;

	while (1)
	{
		if (!PacketRecv(sock, buf))
		{
			break;
		}

		PROTOCOL protocol = GetProtocol(buf);

		switch (protocol)
		{
		case INTRO:		
			{				
				char msg[BUFSIZE];	

				memset(msg, 0, sizeof(msg));
				UnPackPacket(buf, msg);


				int count;
				
				// 전송할 파일 리스트 출력
				while (true)
				{
					printf("%s\n", msg);

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

				strcpy(Fileinfo.filename, argv[count]);

				fp = fopen(Fileinfo.filename, "rb");
				if (!fp)
				{
					err_quit("fopen");
				}

				fseek(fp, 0, SEEK_END);
				Fileinfo.filesize = ftell(fp);
				fclose(fp);

				int size=PackPacket(buf, FILE_INFO, Fileinfo.filename, Fileinfo.filesize);

				retval = send(sock, buf, size, 0);
				if (retval == SOCKET_ERROR)
				{
					err_quit("file info send()");
				}
			}

			break;
		case FILE_TRANS_DENY:
			{
				char msg[BUFSIZE];
				int deny_code;
				memset(msg, 0, sizeof(msg));
				UnPackPacket(buf, deny_code, msg);
				switch (deny_code)
				{
				case FILEEXIST:
					printf("%s\n", msg);
					endflag = true;
					break;
				}
			}
			
			break;
		case FILE_TRANS_START_POINT:
			{
				
				UnPackPacket(buf, Fileinfo.nowsize);

				fp = fopen(Fileinfo.filename, "rb");
				fseek(fp, Fileinfo.nowsize, SEEK_SET);

				while (1)
				{
					char rbuf[BUFSIZE];
					int trans_size = fread(rbuf, 1, MAX_FILE_READ_SIZE, fp);
					if (trans_size > 0)
					{
						int size=PackPacket(buf, FILE_TRANS_DATA, trans_size, rbuf);
						retval = send(sock, buf, size, 0);
						if (retval == SOCKET_ERROR)
						{
							err_display("send()");
							break;
						}
						Fileinfo.nowsize += trans_size;
						printf("..");
						Sleep(200);
					}
					else if (trans_size == 0 && Fileinfo.nowsize == Fileinfo.filesize)
					{
						printf("파일 전송 완료!: %d 바이트\n", Fileinfo.filesize);
						endflag = true;
						break;
					}
					else
					{
						perror("파일 입출력 오류");
						break;
					}
				}

				fclose(fp);

			}			
			break;
		}

		int answer;
		if (endflag)
		{
			printf("계속 하시겠습니까? 1. 네 / 2. 아니오 >> ");
			scanf("%d", &answer);

			int req_code;

			if (answer == 1)
			{
				endflag = false;
				req_code = CONTINUE;
			}
			else
			{
				req_code = END;
			}

			int size = PackPacket(buf, REQUEST, req_code);
			retval = send(sock, buf, size, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				break;
			}

			if (req_code == END)
				break;
		}
	}
	// closesocket()
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	system("pause");
	return 0;
}

// 소켓 함수 오류 출력 후 종료
void err_quit(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(-1);
}

// 소켓 함수 오류 출력
void err_display(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (LPCTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
}


// 사용자 정의 데이터 수신 함수
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

int PackPacket(char* _buf, PROTOCOL _protocol, const char* _str1, int _data)
{
	char* ptr = _buf;
	int strsize = strlen(_str1);
	int size = 0;

	ptr = ptr + sizeof(size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	size = size + sizeof(_protocol);

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

int PackPacket(char* _buf, PROTOCOL _protocol, int _datasize, const char* _filedata)
{
	char* ptr = _buf;
	int size = 0;

	ptr = ptr + sizeof(size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	size = size + sizeof(_protocol);

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

int PackPacket(char* _buf, PROTOCOL _protocol, int code)
{
	int size = 0;
	char* ptr = _buf + sizeof(int);

	memcpy(ptr, &_protocol, sizeof(PROTOCOL));
	size += sizeof(PROTOCOL);
	ptr += sizeof(PROTOCOL);

	memcpy(ptr, &code, sizeof(int));
	size += sizeof(int);
	
	ptr = _buf;
	memcpy(ptr, &size, sizeof(int));

	return size + sizeof(int);
}

PROTOCOL GetProtocol(const char* _buf)
{
	PROTOCOL protocol;
	memcpy(&protocol, _buf, sizeof(PROTOCOL));
	return protocol;
}

void UnPackPacket(const char* _buf, int& _data, char* _str)
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

void UnPackPacket(const char* _buf, char* _str)
{
	const char* ptr = _buf;
	int strsize;

	ptr = ptr + sizeof(PROTOCOL);

	memcpy(&strsize, ptr, sizeof(strsize));
	ptr = ptr + sizeof(strsize);

	memcpy(_str, ptr, strsize);
}

void UnPackPacket(const char* _buf, int& _data)
{
	const char* ptr = _buf + sizeof(PROTOCOL);

	memcpy(&_data, ptr, sizeof(_data));
}

bool PacketRecv(SOCKET _sock, char* _buf)
{
	int size;

	int retval = recvn(_sock, (char*)&size, sizeof(size), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("gvalue recv error()");
		return false;
	}
	else if (retval == 0)
	{
		return false;
	}

	retval = recvn(_sock, _buf, size, 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("gvalue recv error()");
		return false;

	}
	else if (retval == 0)
	{
		return false;
	}

	return true;
}