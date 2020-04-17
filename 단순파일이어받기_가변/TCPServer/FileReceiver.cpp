#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define BUFSIZE 4096

// ���� �Լ� ���� ��� �� ����
void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


// ����� ���� ������ ���� �Լ�
int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while(left > 0){
		received = recv(s, ptr, left, flags);
		if(received == SOCKET_ERROR) 
			return SOCKET_ERROR;
		else if(received == 0) 
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

// ���� ���丮���� ���� ���� ���θ� Ȯ���Ѵ�.
BOOL SearchFile(char *filename)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFindFile = FindFirstFile(filename, &FindFileData);
	if(hFindFile == INVALID_HANDLE_VALUE)
		return FALSE;
	else{
		FindClose(hFindFile);
		return TRUE;
	}
}

int main(int argc, char* argv[])
{
	int retval;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return -1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == INVALID_SOCKET) ErrorHandling("socket()");	
	
	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) ErrorHandling("connect()");

	
		// ���� �̸� �ޱ�
		char filename[256];
		ZeroMemory(filename, 256);
		retval = recvn(sock, filename, 256, 0);
		if(retval == SOCKET_ERROR){
			ErrorHandling ("recv()");
			}
		printf("-> ���� ���� �̸�: %s\n", filename);

		// ���� �˻�
		int currbytes = 0;
		if(SearchFile(filename)){
			// ������ ���� ũ�� ���
			FILE *fp = fopen(filename, "rb");
			if(fp == NULL){
				ErrorHandling("���� ����� ����");
				}
			fseek(fp, 0, SEEK_END);
			currbytes = ftell(fp);
			fclose(fp);
		}

		// ���� ������ ��ġ(=������ ���� ũ��) ������
		retval = send(sock, (char *)&currbytes, sizeof(currbytes), 0);
		if(retval == SOCKET_ERROR){
			ErrorHandling("send()");
			}			

		// ���� ���� ������ ũ�� �ޱ�
		int totalbytes;
		retval = recvn(sock, (char *)&totalbytes, sizeof(totalbytes), 0);
		if(retval == SOCKET_ERROR){
			ErrorHandling("recv()");
			}
		printf("-> ���� ������ ũ��: %d\n", totalbytes);

		// ���� ����
		FILE *fp = fopen(filename, "ab"); // append & binary mode
		if(fp == NULL){
			ErrorHandling("���� ����� ����");
		}

		// ���� ������ �ޱ�
		int numtotal = 0;
		char buf[BUFSIZE];
		while(1){
			retval = recvn(sock, buf, BUFSIZE, 0);
			if(retval == SOCKET_ERROR){
				ErrorHandling("recv()");
				}
			else if(retval == 0) 
				break;
			else{
				fwrite(buf, 1, retval, fp);
				if(ferror(fp)){
					ErrorHandling("���� ����� ����");
					}
				numtotal += retval;
			}
		}
		fclose(fp);

			// closesocket()
		closesocket(sock);
	
	
	// ���� ����
	WSACleanup();
	return 0;
}