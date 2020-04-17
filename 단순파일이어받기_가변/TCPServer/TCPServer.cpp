#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <ws2tcpip.h> 

#define SERVERPORT 9000
#define BUFSIZE 4096

enum PROTOCOL {
	NONEFILE=1,
	FILEEXIST,
	FILELACK,
	SENDFILE,
	END
};

typedef struct clientinfo {
	SOCKET sock;
	SOCKADDR_IN addr;
	char buf[5000];
	char filename[256];
}_ClientInfo;

_ClientInfo* ClientInfo[100];
int Count = 0;

_ClientInfo* AddClientInfo(SOCKET _sock, SOCKADDR_IN _addr)
{
	_ClientInfo* ptr = new _ClientInfo;
	ZeroMemory(ptr, sizeof(_ClientInfo));

	ptr->sock = _sock;
	memcpy(&ptr->addr, &_addr, sizeof(SOCKADDR_IN));

	ZeroMemory(ptr->buf, 5000);
	ZeroMemory(ptr->filename, 256);

	printf("클라이언트 접속:%s :%d\n",
		inet_ntoa(ptr->addr.sin_addr),
		ntohs(ptr->addr.sin_port));

	ClientInfo[Count++] = ptr;
	return ptr;
}

void RemoveClientInfo(_ClientInfo* _ptr)
{
	closesocket(_ptr->sock);

	printf("클라이언트 종료:%s :%d\n",
		inet_ntoa(_ptr->addr.sin_addr),
		ntohs(_ptr->addr.sin_port));

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
}

// 소켓 함수 오류 출력 후 종료
void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// 현재 디렉토리에서 파일 존재 여부를 확인한다.
BOOL SearchFile(const char *filename)
{
	WIN32_FIND_DATA FindFileData;		// 파일 담을 구조체
	HANDLE hFindFile = FindFirstFile(filename, &FindFileData);	// 포인터넘김
	if (hFindFile == INVALID_HANDLE_VALUE)
		return FALSE;
	else {
		FindClose(hFindFile);
		return TRUE;
	}
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

int PackPacket(char* buf, PROTOCOL protocol, int bytes)
{
	int size = 0;
	char* ptr = buf + sizeof(int);

	memcpy(ptr, &protocol, sizeof(PROTOCOL));
	size += sizeof(PROTOCOL);
	ptr += sizeof(PROTOCOL);

	memcpy(ptr, &bytes, sizeof(int));
	size += sizeof(int);

	ptr = buf;
	memcpy(ptr, &size, sizeof(int));

	return size + sizeof(int);
}

void UnPackPacket(const char* buf, char* _str, int *bytes)
{
	int strsize = strlen(_str);
	const char* ptr = buf + sizeof(PROTOCOL);

	memcpy(&strsize, ptr, sizeof(int));
	ptr = ptr + sizeof(int);

	memcpy(_str, ptr, strsize);
	ptr = ptr + strsize;

	memcpy(bytes, ptr, sizeof(int));
}

int UnPackPacket(const char* buf, char* filebuf)
{
	const char* ptr = buf + sizeof(PROTOCOL);
	int bytes;

	memcpy(&bytes, ptr, sizeof(int));
	ptr += sizeof(int);

	memcpy(filebuf, ptr, bytes);

	return bytes;
}

PROTOCOL GetProtocol(char* buf)
{
	PROTOCOL protocol;
	char* ptr = buf;
	memcpy(&protocol, ptr, sizeof(PROTOCOL));

	return protocol;
}

int main(int argc, char *argv[])
{
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
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if(retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;

	while(1)
	{
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
		if(client_sock == INVALID_SOCKET){
			err_display("accept()");
			break;
		}

		_ClientInfo* ptr = AddClientInfo(client_sock, clientaddr);

		// 클라이언트와 데이터 통신
		while(1)
		{
			int totalbytes;

			if (!PacketRecv(ptr->sock, ptr->buf))
			{
				break;
			}

			PROTOCOL protocol = GetProtocol(ptr->buf);
			UnPackPacket(ptr->buf, ptr->filename, &totalbytes);	//파일이름 받기

			printf("-> 받을 파일 이름: %s\n", ptr->filename);			
			printf("총 파일 크기: %d\n", totalbytes);

			// 파일 검색
			int currbytes = 0;
			if (SearchFile(ptr->filename)) {
				// 현재의 파일 크기 얻기 (서버가 가진)
				FILE* fp = fopen(ptr->filename, "rb");
				if (fp == NULL) {
					err_display("파일 입출력 오류");
				}
				fseek(fp, 0, SEEK_END); // 위치포인터, 옮길만큼, 기준점
				currbytes = ftell(fp);  // 위치포인터에서 ~ 시작지점 까지 잰다
				fclose(fp);

				if (totalbytes == currbytes)
					protocol = FILEEXIST;		// 이미 받은 파일
				else if (totalbytes > currbytes)
					protocol = FILELACK;		// 이어 받아야 하는 파일
			}
			else
				protocol = NONEFILE;			// 존재하지 않는 파일

			int size = PackPacket(ptr->buf, protocol, currbytes);

			// 전송 시작할 위치(=현재의 파일 크기) 보내기
			retval = send(ptr->sock, ptr->buf, size, 0);	// 현재 서버가 가진 파일의 유무
															// 파일의 크기 currbytes
			if (retval == SOCKET_ERROR) {
				err_display("send()");
			}
			printf("-> 받을 데이터 크기: %d\n", totalbytes - currbytes);


			// 파일 열기
			FILE* fp = fopen(ptr->filename, "ab"); // append & binary mode // 위치포인터 파일의 끝
			if (fp == NULL) {
				err_display("파일 입출력 오류1");
			}

			// 파일 데이터 받기
			int numtotal = 0;
			bool endflag = false;

			while (1)
			{
				if (!PacketRecv(ptr->sock, ptr->buf))
				{
					break;
				}
				protocol = GetProtocol(ptr->buf);

				char file[BUFSIZE];
				ZeroMemory(file, BUFSIZE);
				int bytes;

				switch (protocol)
				{
				case SENDFILE:
					bytes = UnPackPacket(ptr->buf, file);

					fwrite(file, 1, bytes, fp);
					if (ferror(fp)) {
						err_display("파일 입출력 오류");
					}
					numtotal += bytes;

					break;
				case END:
					endflag = true;
					break;
				}
				
				if (endflag)
				{
					printf("파일 받기 완료\n");
					break;
				}
			}

			fclose(fp);

			// closesocket()
			RemoveClientInfo(ptr);
			break;
		}

		// closesocket()
	//	closesocket(client_sock);
		
	}

	// closesocket()
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}