#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define LOCALPORT   9001
#define REMOTEPORT 9001
#define BUFSIZE    512

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

struct SockInfo
{
	SOCKET sock;				// ����
	SOCKADDR_IN sockaddr;		// ���� �ּ� ����ü
	ip_mreq mreq;
};

// ������ ��� ���� �÷���
bool Exit = false;

// ���� �����
SockInfo* CreateNormalSocket();								// �ܼ� UDPŬ���̾�Ʈ ���� ����
SockInfo* CreateMRecvSocket(const char* MULTICASTIP);		// ��Ƽĳ��Ʈ �ޱ� ���� ����
SockInfo* CreateMSendSocket(const char* MULTICASTIP);		// ��Ƽĳ��Ʈ ������ ���� ����

// ������
DWORD WINAPI SendThread(LPVOID);
DWORD WINAPI ReceiveThread(LPVOID);

int main(int argc, char *argv[])
{
	int retval;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	// �ܼ� udp����� ���� ���� ����
	SockInfo* normalSock = CreateNormalSocket();

	// ������ ��ſ� ����� ����
	SOCKADDR_IN peeraddr;
	int addrlen;
	char buf[BUFSIZE+1];
	int len;
	char MULTICASTIP[50];

	int select = 0;

	// ���� ���� ����
	printf("0. ����Ʈ �ޱ�\n");
	printf("1. 1���� �ּ� �ޱ�\n");
	printf("2. 2���� �ּ� �ޱ�\n");
	printf("3. 3���� �ּ� �ޱ�\n");
	printf("���� >>");
	if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
		return 0;

	// '\n' ���� ����
	len = strlen(buf);
	if (buf[len - 1] == '\n')
		buf[len - 1] = '\0';
	if (strlen(buf) == 0)
		return 0;

	select = atoi(buf);

	// ������ ������ - ���� ��Ŷ ������
	retval = sendto(normalSock->sock, buf, strlen(buf), 0,
		(SOCKADDR*)&normalSock->sockaddr, sizeof(normalSock->sockaddr));
	if (retval == SOCKET_ERROR) {
		err_display("sendto()");
		//continue;
	}

	// �����Ѱ� �޾ƿ���
	addrlen = sizeof(peeraddr);
	retval = recvfrom(normalSock->sock, buf, BUFSIZE, 0,
		(SOCKADDR*)&peeraddr, &addrlen);
	if (retval == SOCKET_ERROR) {
		err_display("recvfrom()");
	}
	
	buf[retval] = '\0';

	switch (select)
	{
	case 0:		// ����Ʈ �޾ƿ���
		printf("%s\n", buf);
		printf("���� >>");

		if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
			return 0;

		// '\n' ���� ����
		len = strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0)
			return 0;

		// ������ ������ - ���� ��Ŷ ������
		retval = sendto(normalSock->sock, buf, strlen(buf), 0,
			(SOCKADDR*)&peeraddr, sizeof(peeraddr));
		if (retval == SOCKET_ERROR) {
			err_display("sendto()");
			//continue;
		}

		memset(buf, 0, BUFSIZE);

		// �����Ѱ� �޾ƿ���
		addrlen = sizeof(peeraddr);
		retval = recvfrom(normalSock->sock, buf, BUFSIZE, 0,
			(SOCKADDR*)&peeraddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
		}

		printf("������ : %s\n", buf);

		break;
	case 1:
		strcpy(MULTICASTIP, buf);

		printf("������ : %s\n", MULTICASTIP);
		break;
	case 2:
		strcpy(MULTICASTIP, buf);

		printf("������ : %s\n", MULTICASTIP);
		break;
	case 3:
		strcpy(MULTICASTIP, buf);

		printf("������ : %s\n", MULTICASTIP);
		break;
	}

	/*
		��Ƽĳ��Ʈ ����
	*/

	// ��Ƽĳ��Ʈ�� ���� ���� ����
	SockInfo* mrecvSock = CreateMRecvSocket(MULTICASTIP);		// ��Ƽĳ��Ʈ �ޱ�� ���� (bind)
	SockInfo* msendSock = CreateMSendSocket(MULTICASTIP);		// ��Ƽĳ��Ʈ ������� ����

	// ������ ����
	HANDLE Send = CreateThread(NULL, 0, SendThread, msendSock, 0, NULL);
	HANDLE Receive = CreateThread(NULL, 0, ReceiveThread, mrecvSock, 0, NULL);

	// ������ ���
	while (1)
	{
		if (Exit)
			break;
		else
			Sleep(2000);
	}

	system("pause");
	// closesocket()
	closesocket(normalSock->sock);

	// ���� ����
	WSACleanup();
	return 0;
}

DWORD WINAPI SendThread(LPVOID _ptr)
{
	SockInfo* info = (SockInfo*)_ptr;

	int retval;
	char buf[BUFSIZE + 1];
	int len;

	while (1)
	{
		// ������ �Է�
		printf("\n[���� ������] ");
		if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
			break;

		// '\n' ���� ����
		len = strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0)
			break;

		// ������ ������
		retval = sendto(info->sock, buf, strlen(buf), 0,
			(SOCKADDR*)&info->sockaddr, sizeof(info->sockaddr));
		if (retval == SOCKET_ERROR) {
			err_display("sendto()");
			continue;
		}
	}

	return 0;
}

DWORD WINAPI ReceiveThread(LPVOID _ptr)
{
	SockInfo* info = (SockInfo*)_ptr;

	int retval;
	SOCKADDR_IN peeraddr;
	int addrlen;
	char buf[BUFSIZE + 1];

	while (1) {
		// ������ �ޱ�
		addrlen = sizeof(peeraddr);
		retval = recvfrom(info->sock, buf, BUFSIZE, 0,
			(SOCKADDR*)&peeraddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
			continue;
		}

		// ���� ������ ���
		buf[retval] = '\0';
		printf("[UDP/%s:%d] %s\n", inet_ntoa(peeraddr.sin_addr),
			ntohs(peeraddr.sin_port), buf);
	}

	// ��Ƽĳ��Ʈ �׷� Ż��
	retval = setsockopt(info->sock, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		(char*)&info->mreq, sizeof(info->mreq));
	if (retval == SOCKET_ERROR) err_quit("setsockopt()");

	Exit = true;

	return 0;
}

SockInfo* CreateNormalSocket()
{
	SockInfo* sockinfo = new SockInfo();
	ZeroMemory(sockinfo, sizeof(sockinfo));

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// ���� �ּ� ����ü �ʱ�ȭ
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);

	sockinfo->sock = sock;
	memcpy(&sockinfo->sockaddr, &serveraddr, sizeof(SOCKADDR_IN));

	return sockinfo;
}

SockInfo* CreateMRecvSocket(const char* MULTICASTIP)
{
	SockInfo* sockinfo = new SockInfo();
	ZeroMemory(sockinfo, sizeof(sockinfo));

	int retval;

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// SO_REUSEADDR �ɼ� ����
	BOOL optval = TRUE;
	retval = setsockopt(sock, SOL_SOCKET,
		SO_REUSEADDR, (char*)&optval, sizeof(optval));
	if (retval == SOCKET_ERROR) err_quit("setsockopt()");

	// bind()
	SOCKADDR_IN localaddr;
	ZeroMemory(&localaddr, sizeof(localaddr));
	localaddr.sin_family = AF_INET;
	localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localaddr.sin_port = htons(LOCALPORT);
	retval = bind(sock, (SOCKADDR*)&localaddr, sizeof(localaddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// ��Ƽĳ��Ʈ �׷� ����
	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr(MULTICASTIP);		// ���� �ּҷ� ����
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);			// �� �ּҰ� ����
	retval = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		(char*)&mreq, sizeof(mreq));
	if (retval == SOCKET_ERROR) err_quit("setsockopt()");

	sockinfo->sock = sock;
	memcpy(&sockinfo->sockaddr, &localaddr, sizeof(SOCKADDR_IN));
	sockinfo->mreq = mreq;

	return sockinfo;
}

SockInfo* CreateMSendSocket(const char* MULTICASTIP)
{
	SockInfo* sockinfo = new SockInfo();
	ZeroMemory(sockinfo, sizeof(sockinfo));

	int retval;

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// ��Ƽĳ��Ʈ TTL ����
	int ttl = 2;		// ����͸� � ���������� �ִ���??  ����� ���������� -1 -> 0�Ǹ� ����
	retval = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL,		// time to live? �������
		(char*)&ttl, sizeof(ttl));
	if (retval == SOCKET_ERROR) err_quit("setsockopt()");

	// ���� �ּ� ����ü �ʱ�ȭ
	SOCKADDR_IN remoteaddr;
	ZeroMemory(&remoteaddr, sizeof(remoteaddr));
	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_addr.s_addr = inet_addr(MULTICASTIP);
	remoteaddr.sin_port = htons(REMOTEPORT);

	sockinfo->sock = sock;
	memcpy(&sockinfo->sockaddr, &remoteaddr, sizeof(SOCKADDR_IN));

	return sockinfo;
}