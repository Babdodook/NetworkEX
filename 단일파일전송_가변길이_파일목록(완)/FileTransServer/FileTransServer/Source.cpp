#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define BUFSIZE 4096
#define FILENAMESIZE 256
#define MAXCONCLIENT 100

#define INTRO_MSG "전송할 파일명을 선택하세요"
#define FILE_EXIST_MSG "전송하고자 하는 파일은 이미 서버에 존재하는 파일입니다."

enum PROTOCOL 
{ 
	INTRO=1, 
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
	CONTINUE = 1,
	END
};

struct _File_info
{
	char filename[FILENAMESIZE];
	int  filesize;	
	int  nowsize;
};


struct _ClientInfo
{
	SOCKET sock;
	SOCKADDR_IN addr;
	_File_info  file_info;
	char packetbuf[BUFSIZE];
};

void err_quit(const char*);
void err_display(const char*);

int recvn(SOCKET, char*, int, int);

_ClientInfo* AddClientInfo(SOCKET sock, SOCKADDR_IN addr);
void ReMoveClientInfo(_ClientInfo*);

bool SearchFile(const char*);
bool PacketRecv(SOCKET, char*);
PROTOCOL GetProtocol(const char*);
int PackPacket(char*, PROTOCOL, const char*);
int PackPacket(char*, PROTOCOL, int, const char*);
int PackPacket(char*, PROTOCOL, int);



void UnPackPacket(const char* _buf, char* _str1, int& _data1);
void UnPackPacket(const char*, int&, char*);
void UnPackPacket(const char*, int&);

_ClientInfo* ClientInfo[MAXCONCLIENT];
int count;

int main(int argc, char* argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수

	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	
	
	char buf[BUFSIZE];
	FILE *fp=nullptr;

	while (1)
	{
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			continue;
		}	

		_ClientInfo* ClientPtr = AddClientInfo(client_sock, clientaddr);		
		
		int size = PackPacket(ClientPtr->packetbuf, INTRO, INTRO_MSG);

		retval = send(ClientPtr->sock, ClientPtr->packetbuf, size, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
			ReMoveClientInfo(ClientPtr);
			continue;
		}
		
		bool fileflag = false;
		bool endflag = false;
		bool sendfail = false;

		while (1)
		{
			if (!PacketRecv(ClientPtr->sock, ClientPtr->packetbuf))
			{
				break;
			}

			PROTOCOL protocol = GetProtocol(ClientPtr->packetbuf);

			switch (protocol)
			{
			case FILE_INFO:
				{
					char filename[FILENAMESIZE];
					int filesize;
					memset(filename, 0, sizeof(filename));
					
					UnPackPacket(ClientPtr->packetbuf, filename, filesize);

					printf("-> 받을 파일 이름: %s\n", filename);
					printf("-> 받을 파일 크기: %d\n", filesize);

					if (SearchFile(filename))
					{
						fp = fopen(filename, "rb");
						fseek(fp, 0, SEEK_END);
						int fsize = ftell(fp);
						fclose(fp);
						if (filesize == fsize)
						{
							printf("존재하는 파일 전송 요구\n");

							int size=PackPacket(ClientPtr->packetbuf, FILE_TRANS_DENY, FILEEXIST, FILE_EXIST_MSG);
							sendfail = true;

							retval = send(ClientPtr->sock, ClientPtr->packetbuf, size, 0);
							if (retval == SOCKET_ERROR)
							{
								err_display("send()");
							}
							break;
						}
						else
						{
							strcpy(ClientPtr->file_info.filename, filename);
							ClientPtr->file_info.filesize = filesize;
							ClientPtr->file_info.nowsize = fsize;
						}

					}
					else
					{
						strcpy(ClientPtr->file_info.filename, filename);
						ClientPtr->file_info.filesize = filesize;
						ClientPtr->file_info.nowsize = 0;
					}			
					
					int size=PackPacket(ClientPtr->packetbuf, FILE_TRANS_START_POINT, ClientPtr->file_info.nowsize);

					retval = send(ClientPtr->sock, ClientPtr->packetbuf, size, 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("send()");
						endflag = true;
						break;
					}
				}
				break;
			case FILE_TRANS_DATA:
				{
					if (!fileflag)
					{
						fileflag = true;
						fp = fopen(ClientPtr->file_info.filename, "ab");
					}

					int transsize;
					UnPackPacket(ClientPtr->packetbuf, transsize, buf);
					fwrite(buf, 1, transsize, fp);
					if (ferror(fp)) {
						perror("파일 입출력 오류");
						endflag = true;
						break;
					}
					ClientPtr->file_info.nowsize += transsize;
				}
				break;
			case REQUEST:
				ZeroMemory(&ClientPtr->file_info, sizeof(_File_info));

				fclose(fp);
				fileflag = false;

				int req_code;
				UnPackPacket(ClientPtr->packetbuf, req_code);

				if (req_code == END)
				{
					endflag = true;
				}
				else if(req_code == CONTINUE)
				{
					int size = PackPacket(ClientPtr->packetbuf, INTRO, INTRO_MSG);

					retval = send(ClientPtr->sock, ClientPtr->packetbuf, size, 0);
					if (retval == SOCKET_ERROR)
					{
						err_display("send()");
						ReMoveClientInfo(ClientPtr);
						continue;
					}
				}

				break;
			}//switch end

			if (ClientPtr->file_info.filesize != 0 && ClientPtr->file_info.filesize == ClientPtr->file_info.nowsize)
			{
				printf("전송완료!!\n");
			}
			else if(sendfail)
			{
				sendfail = false;
				printf("전송실패!!\n");
			}

			if (endflag)
			{
				break;
			}

		}//inner while end

		ReMoveClientInfo(ClientPtr);

	}//outer while end

	 // closesocket()
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
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


_ClientInfo* AddClientInfo(SOCKET sock, SOCKADDR_IN addr)
{
	_ClientInfo* ptr = new _ClientInfo;
	ZeroMemory(ptr, sizeof(_ClientInfo));
	ptr->sock = sock;
	memcpy(&ptr->addr, &addr, sizeof(addr));
	ClientInfo[count++] = ptr;

	printf("\nFileSender 접속: IP 주소=%s, 포트 번호=%d\n",
		inet_ntoa(ptr->addr.sin_addr), ntohs(ptr->addr.sin_port));

	return ptr;
}

void ReMoveClientInfo(_ClientInfo* ptr)
{
	
	printf("FileSender 종료: IP 주소=%s, 포트 번호=%d\n",
		inet_ntoa(ptr->addr.sin_addr), ntohs(ptr->addr.sin_port));

	for (int i = 0; i<count; i++)
	{
		if (ClientInfo[i] == ptr)
		{
			closesocket(ptr->sock);
			delete ptr;
			for (int j = i; j<count - 1; j++)
			{
				ClientInfo[j] = ClientInfo[j + 1];
			}
			break;
		}
	}

	count--;
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

int PackPacket(char* _buf, PROTOCOL  _protocol, const char* _str) //인트로 
{
	char* ptr = _buf;
	int strsize = strlen(_str);
	int size = 0;

	ptr = ptr + sizeof(size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	size = size + sizeof(_protocol);

	
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

int PackPacket(char* _buf, PROTOCOL  _protocol, int _data, const char* _str) //파일전송 거부 정보
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


int PackPacket(char* _buf, PROTOCOL _protocol, int _data)//파일 전송 위치 정보
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

PROTOCOL GetProtocol(const char* _buf)
{
	PROTOCOL protocol;
	memcpy(&protocol, _buf, sizeof(PROTOCOL));
	return protocol;
}

void UnPackPacket(const char* _buf, char* _str1, int& _data1)//파일이름 & 파일 크기
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

void UnPackPacket(const char* _buf, int& _size, char* _targetbuf) //파일 전송 데이터
{
	const char* ptr = _buf + sizeof(PROTOCOL);

	memcpy(&_size, ptr, sizeof(_size));
	ptr = ptr + sizeof(_size);

	memcpy(_targetbuf, ptr, _size);
}

void UnPackPacket(const char* _buf, int& req_code)
{
	const char* ptr = _buf + sizeof(PROTOCOL);

	memcpy(&req_code, ptr, sizeof(int));
}
