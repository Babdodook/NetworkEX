#include"basic.h"
#include"LinkedList.h"
#include"Packet.h"
#include"menu.h"

int main()
{
	//유저 정보를 저장할 리스트 생성
	_UserList* Userlist = (_UserList*)malloc(sizeof(_UserList));
	List_Init(Userlist);

	//파일에 저장된 정보를 읽어온다
	FILE *fp = fopen("List.txt", "rt");

	if (fp)
	{
		loadList(fp, Userlist);
		fclose(fp);
		printf("불러오기 성공\n");
	}
	else
	{
		printf("불러오기 실패 : 저장된 자료 없음\n");
	}

	srand(time(NULL));

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa)
		!= 0)
	{
		err_quit("WSAStartUP()");
	}

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
	{
		err_quit("socket()");
	}

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SERVERPORT);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int retval = bind(listen_sock,
		(SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		err_quit("bind()");
	}

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
	{
		err_quit("listen()");
	}

	// 클라이언트와 통신할 변수
	SOCKADDR_IN clientaddr;
	SOCKET client_sock;
	int addrlen;
	char buf[BUFSIZE];

	while (1)
	{
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock,
			(SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			err_display("accept()");
			continue;
		}

		printf("클라이언트 접속:%s :%d\n",
			inet_ntoa(clientaddr.sin_addr),
			ntohs(clientaddr.sin_port));

		bool endflag = false;
		_User User;

		// 접속 성공 시 클라이언트와 데이터 통신
		while (1)
		{
			int size;

			//사이즈 먼저 받는다
			int retval = recvn(client_sock, (char*)&size, sizeof(int), 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("recvn_size()");
				break;
			}
			else if (retval == 0)
			{
				break;
			}

			//받은 사이즈만큼 버퍼를 읽는다
			retval = recvn(client_sock, buf, size, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("recvn_buf()");
				break;
			}
			else if (retval == 0)
			{
				break;
			}

			PROTOCOL protocol;
			char* ptr = buf;

			//프로토콜을 먼저 읽음
			memcpy(&protocol, ptr, sizeof(PROTOCOL));
			ptr += sizeof(PROTOCOL);

			int result;
			switch (protocol)
			{
			case P_LOGIN:		//로그인
				result = Login(client_sock, ptr, &User, Userlist);
				if (result == RESULTS::SUCCESS)	//로그인 성공이면 게임 시작
				{
					if (Game(client_sock, ptr, &User, Userlist) == BREAK)
						endflag = true;
				}
				else if(result == BREAK)
					endflag = true;
				break;
			case P_REGISTER:	//회원가입
				result = Register(client_sock, ptr, &User, Userlist);
				if (result == BREAK)
					endflag = true;
				break;
			case P_DELETEACC:	//회원탈퇴
				if (DeleteAcc(client_sock, ptr, &User, Userlist) == BREAK)
					endflag = true;
				break;
			case P_RANK:		//랭크정보
				if (RankInfo(client_sock, Userlist) == BREAK)
					endflag = true;
				break;
			case P_EXIT:		//종료
				endflag = true;
				break;
			}

			//엔드플래그 true면 클라이언트와 연결 종료
			if (endflag)
				break;


		}

		// closesocket()
		closesocket(client_sock);

		printf("클라이언트 종료:%s :%d\n",
			inet_ntoa(clientaddr.sin_addr),
			ntohs(clientaddr.sin_port));
	}

	// closesocket()
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();

	//동적할당 해제
	freeNodes(Userlist->head);
	return 0;
}

