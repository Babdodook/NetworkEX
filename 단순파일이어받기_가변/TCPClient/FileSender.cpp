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

int main(int argc, char* argv[])
{
	int retval;

	if(argc < 2){
		printf("Usage: %s [FileName]\n", argv[0]);
		return -1;
	}

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return -1;

	// socket()
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_sock == INVALID_SOCKET) ErrorHandling("socket()");	
	
	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) ErrorHandling("bind()");
	
	// listen()
	retval = listen(listen_sock, 5);
	if(retval == SOCKET_ERROR) ErrorHandling("listen()");

	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE];

	while(1){
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
		if(client_sock == INVALID_SOCKET){
			ErrorHandling ("accept()");
		}
		printf("\nFileSender ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", 
			inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);

	// ���� ����
	FILE *fp = fopen("VMware-workstation-6.0.0-45731.exe", "rb");
	if(fp == NULL){
		ErrorHandling("���� ����� ����");
		return -1;
	}
	
	// ���� �̸� ������
	char filename[256];
	ZeroMemory(filename, 256);
	sprintf(filename, "VMware-workstation-6.0.0-45731.exe");
	retval = send(client_sock, filename, 256, 0);
	if(retval == SOCKET_ERROR) ErrorHandling("send()");	

	// ���� ������ ��ġ(=������ ���� ũ��) �ޱ�
	int currbytes;
	retval = recvn(client_sock, (char *)&currbytes, sizeof(currbytes), 0);
	if(retval == SOCKET_ERROR) ErrorHandling("recv()");

	// ���� ũ�� ���
	fseek(fp, 0, SEEK_END);
	int totalbytes = ftell(fp);
	totalbytes -= currbytes;
	
	// ������ ������ ũ�� ������
	retval = send(client_sock, (char *)&totalbytes, sizeof(totalbytes), 0);
	if(retval == SOCKET_ERROR) ErrorHandling("send()");	

	// ���� ������ ���ۿ� ����� ����
	char buf[BUFSIZE];
	int numread;
	int numtotal = 0;

	// ���� ������ ������
	fseek(fp, currbytes, SEEK_SET); // ���� �����͸� ���� ���� ��ġ�� �̵�
	while(1){
		numread = fread(buf, 1, BUFSIZE, fp);
		if(numread > 0){
			retval = send(client_sock, buf, numread, 0);
			if(retval == SOCKET_ERROR){
				ErrorHandling("send()");
			}
			numtotal += numread;
			printf("."); // ���� ��Ȳ�� ǥ��
			Sleep(50); // ���� �ߴ� ������ ���� �ӵ��� ������ ��
		}
		else if(numread == 0 && numtotal == totalbytes){
			printf("\n���� ���� �Ϸ�!: %d ����Ʈ\n", numtotal);
			break;
		}
		else{
			ErrorHandling("���� ����� ����");
		}
	}
	fclose(fp);
    closesocket(client_sock);
		printf("FileSender ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", 
		inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
	}
	
	closesocket(listen_sock);
	

	// ���� ����
	WSACleanup();
	return 0;
}