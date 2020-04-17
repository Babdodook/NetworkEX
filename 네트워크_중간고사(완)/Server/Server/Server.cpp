#include"basic.h"
#include"LinkedList.h"
#include"Packet.h"
#include"menu.h"

int main()
{
	//���� ������ ������ ����Ʈ ����
	_UserList* Userlist = (_UserList*)malloc(sizeof(_UserList));
	List_Init(Userlist);

	//���Ͽ� ����� ������ �о�´�
	FILE *fp = fopen("List.txt", "rt");

	if (fp)
	{
		loadList(fp, Userlist);
		fclose(fp);
		printf("�ҷ����� ����\n");
	}
	else
	{
		printf("�ҷ����� ���� : ����� �ڷ� ����\n");
	}

	srand(time(NULL));

	// ���� �ʱ�ȭ
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

	// Ŭ���̾�Ʈ�� ����� ����
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

		printf("Ŭ���̾�Ʈ ����:%s :%d\n",
			inet_ntoa(clientaddr.sin_addr),
			ntohs(clientaddr.sin_port));

		bool endflag = false;
		_User User;

		// ���� ���� �� Ŭ���̾�Ʈ�� ������ ���
		while (1)
		{
			int size;

			//������ ���� �޴´�
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

			//���� �����ŭ ���۸� �д´�
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

			//���������� ���� ����
			memcpy(&protocol, ptr, sizeof(PROTOCOL));
			ptr += sizeof(PROTOCOL);

			int result;
			switch (protocol)
			{
			case P_LOGIN:		//�α���
				result = Login(client_sock, ptr, &User, Userlist);
				if (result == RESULTS::SUCCESS)	//�α��� �����̸� ���� ����
				{
					if (Game(client_sock, ptr, &User, Userlist) == BREAK)
						endflag = true;
				}
				else if(result == BREAK)
					endflag = true;
				break;
			case P_REGISTER:	//ȸ������
				result = Register(client_sock, ptr, &User, Userlist);
				if (result == BREAK)
					endflag = true;
				break;
			case P_DELETEACC:	//ȸ��Ż��
				if (DeleteAcc(client_sock, ptr, &User, Userlist) == BREAK)
					endflag = true;
				break;
			case P_RANK:		//��ũ����
				if (RankInfo(client_sock, Userlist) == BREAK)
					endflag = true;
				break;
			case P_EXIT:		//����
				endflag = true;
				break;
			}

			//�����÷��� true�� Ŭ���̾�Ʈ�� ���� ����
			if (endflag)
				break;


		}

		// closesocket()
		closesocket(client_sock);

		printf("Ŭ���̾�Ʈ ����:%s :%d\n",
			inet_ntoa(clientaddr.sin_addr),
			ntohs(clientaddr.sin_port));
	}

	// closesocket()
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();

	//�����Ҵ� ����
	freeNodes(Userlist->head);
	return 0;
}

