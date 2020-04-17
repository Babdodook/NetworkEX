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

	printf("Ŭ���̾�Ʈ ����:%s :%d\n",
		inet_ntoa(ptr->addr.sin_addr),
		ntohs(ptr->addr.sin_port));

	ClientInfo[Count++] = ptr;
	return ptr;
}

void RemoveClientInfo(_ClientInfo* _ptr)
{
	closesocket(_ptr->sock);

	printf("Ŭ���̾�Ʈ ����:%s :%d\n",
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

// ���� �Լ� ���� ��� �� ����
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

// ���� �Լ� ���� ���
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

// ���� ���丮���� ���� ���� ���θ� Ȯ���Ѵ�.
BOOL SearchFile(const char *filename)
{
	WIN32_FIND_DATA FindFileData;		// ���� ���� ����ü
	HANDLE hFindFile = FindFirstFile(filename, &FindFileData);	// �����ͳѱ�
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

	// ���� �ʱ�ȭ
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

	// ������ ��ſ� ����� ����
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

		// Ŭ���̾�Ʈ�� ������ ���
		while(1)
		{
			int totalbytes;

			if (!PacketRecv(ptr->sock, ptr->buf))
			{
				break;
			}

			PROTOCOL protocol = GetProtocol(ptr->buf);
			UnPackPacket(ptr->buf, ptr->filename, &totalbytes);	//�����̸� �ޱ�

			printf("-> ���� ���� �̸�: %s\n", ptr->filename);			
			printf("�� ���� ũ��: %d\n", totalbytes);

			// ���� �˻�
			int currbytes = 0;
			if (SearchFile(ptr->filename)) {
				// ������ ���� ũ�� ��� (������ ����)
				FILE* fp = fopen(ptr->filename, "rb");
				if (fp == NULL) {
					err_display("���� ����� ����");
				}
				fseek(fp, 0, SEEK_END); // ��ġ������, �ű游ŭ, ������
				currbytes = ftell(fp);  // ��ġ�����Ϳ��� ~ �������� ���� ���
				fclose(fp);

				if (totalbytes == currbytes)
					protocol = FILEEXIST;		// �̹� ���� ����
				else if (totalbytes > currbytes)
					protocol = FILELACK;		// �̾� �޾ƾ� �ϴ� ����
			}
			else
				protocol = NONEFILE;			// �������� �ʴ� ����

			int size = PackPacket(ptr->buf, protocol, currbytes);

			// ���� ������ ��ġ(=������ ���� ũ��) ������
			retval = send(ptr->sock, ptr->buf, size, 0);	// ���� ������ ���� ������ ����
															// ������ ũ�� currbytes
			if (retval == SOCKET_ERROR) {
				err_display("send()");
			}
			printf("-> ���� ������ ũ��: %d\n", totalbytes - currbytes);


			// ���� ����
			FILE* fp = fopen(ptr->filename, "ab"); // append & binary mode // ��ġ������ ������ ��
			if (fp == NULL) {
				err_display("���� ����� ����1");
			}

			// ���� ������ �ޱ�
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
						err_display("���� ����� ����");
					}
					numtotal += bytes;

					break;
				case END:
					endflag = true;
					break;
				}
				
				if (endflag)
				{
					printf("���� �ޱ� �Ϸ�\n");
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

	// ���� ����
	WSACleanup();
	return 0;
}