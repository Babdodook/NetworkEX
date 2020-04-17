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

struct SockInfo
{
	SOCKET sock;				// 소켓
	SOCKADDR_IN sockaddr;		// 소켓 주소 구조체
	ip_mreq mreq;
};

// 무한정 대기 위한 플래그
bool Exit = false;

// 소켓 만들기
SockInfo* CreateNormalSocket();								// 단순 UDP클라이언트 소켓 생성
SockInfo* CreateMRecvSocket(const char* MULTICASTIP);		// 멀티캐스트 받기 소켓 생성
SockInfo* CreateMSendSocket(const char* MULTICASTIP);		// 멀티캐스트 보내기 소켓 생성

// 스레드
DWORD WINAPI SendThread(LPVOID);
DWORD WINAPI ReceiveThread(LPVOID);

int main(int argc, char *argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	// 단순 udp통신을 위한 소켓 생성
	SockInfo* normalSock = CreateNormalSocket();

	// 데이터 통신에 사용할 변수
	SOCKADDR_IN peeraddr;
	int addrlen;
	char buf[BUFSIZE+1];
	int len;
	char MULTICASTIP[50];

	int select = 0;

	// 보낼 내용 선택
	printf("0. 리스트 받기\n");
	printf("1. 1번방 주소 받기\n");
	printf("2. 2번방 주소 받기\n");
	printf("3. 3번방 주소 받기\n");
	printf("선택 >>");
	if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
		return 0;

	// '\n' 문자 제거
	len = strlen(buf);
	if (buf[len - 1] == '\n')
		buf[len - 1] = '\0';
	if (strlen(buf) == 0)
		return 0;

	select = atoi(buf);

	// 데이터 보내기 - 선택 패킷 보내기
	retval = sendto(normalSock->sock, buf, strlen(buf), 0,
		(SOCKADDR*)&normalSock->sockaddr, sizeof(normalSock->sockaddr));
	if (retval == SOCKET_ERROR) {
		err_display("sendto()");
		//continue;
	}

	// 선택한거 받아오기
	addrlen = sizeof(peeraddr);
	retval = recvfrom(normalSock->sock, buf, BUFSIZE, 0,
		(SOCKADDR*)&peeraddr, &addrlen);
	if (retval == SOCKET_ERROR) {
		err_display("recvfrom()");
	}
	
	buf[retval] = '\0';

	switch (select)
	{
	case 0:		// 리스트 받아오기
		printf("%s\n", buf);
		printf("선택 >>");

		if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
			return 0;

		// '\n' 문자 제거
		len = strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0)
			return 0;

		// 데이터 보내기 - 선택 패킷 보내기
		retval = sendto(normalSock->sock, buf, strlen(buf), 0,
			(SOCKADDR*)&peeraddr, sizeof(peeraddr));
		if (retval == SOCKET_ERROR) {
			err_display("sendto()");
			//continue;
		}

		memset(buf, 0, BUFSIZE);

		// 선택한거 받아오기
		addrlen = sizeof(peeraddr);
		retval = recvfrom(normalSock->sock, buf, BUFSIZE, 0,
			(SOCKADDR*)&peeraddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
		}

		printf("아이피 : %s\n", buf);

		break;
	case 1:
		strcpy(MULTICASTIP, buf);

		printf("아이피 : %s\n", MULTICASTIP);
		break;
	case 2:
		strcpy(MULTICASTIP, buf);

		printf("아이피 : %s\n", MULTICASTIP);
		break;
	case 3:
		strcpy(MULTICASTIP, buf);

		printf("아이피 : %s\n", MULTICASTIP);
		break;
	}

	/*
		멀티캐스트 시작
	*/

	// 멀티캐스트를 위한 소켓 생성
	SockInfo* mrecvSock = CreateMRecvSocket(MULTICASTIP);		// 멀티캐스트 받기용 소켓 (bind)
	SockInfo* msendSock = CreateMSendSocket(MULTICASTIP);		// 멀티캐스트 보내기용 소켓

	// 스레드 생성
	HANDLE Send = CreateThread(NULL, 0, SendThread, msendSock, 0, NULL);
	HANDLE Receive = CreateThread(NULL, 0, ReceiveThread, mrecvSock, 0, NULL);

	// 무한정 대기
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

	// 윈속 종료
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
		// 데이터 입력
		printf("\n[보낼 데이터] ");
		if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
			break;

		// '\n' 문자 제거
		len = strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0)
			break;

		// 데이터 보내기
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
		// 데이터 받기
		addrlen = sizeof(peeraddr);
		retval = recvfrom(info->sock, buf, BUFSIZE, 0,
			(SOCKADDR*)&peeraddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
			continue;
		}

		// 받은 데이터 출력
		buf[retval] = '\0';
		printf("[UDP/%s:%d] %s\n", inet_ntoa(peeraddr.sin_addr),
			ntohs(peeraddr.sin_port), buf);
	}

	// 멀티캐스트 그룹 탈퇴
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

	// 소켓 주소 구조체 초기화
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

	// SO_REUSEADDR 옵션 설정
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

	// 멀티캐스트 그룹 가입
	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr(MULTICASTIP);		// 여기 주소로 가입
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);			// 이 주소가 위의
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

	// 멀티캐스트 TTL 설정
	int ttl = 2;		// 라우터를 몇개 빠져나갈수 있는지??  라우터 지날때마다 -1 -> 0되면 폐기됨
	retval = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL,		// time to live? 살아있음
		(char*)&ttl, sizeof(ttl));
	if (retval == SOCKET_ERROR) err_quit("setsockopt()");

	// 소켓 주소 구조체 초기화
	SOCKADDR_IN remoteaddr;
	ZeroMemory(&remoteaddr, sizeof(remoteaddr));
	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_addr.s_addr = inet_addr(MULTICASTIP);
	remoteaddr.sin_port = htons(REMOTEPORT);

	sockinfo->sock = sock;
	memcpy(&sockinfo->sockaddr, &remoteaddr, sizeof(SOCKADDR_IN));

	return sockinfo;
}