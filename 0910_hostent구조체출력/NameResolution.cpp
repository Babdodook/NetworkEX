#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdio.h>

#define TESTNAME "www.google.com"

void GetInfo(HOSTENT *ptr);

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

// ������ �̸� -> IPv4 �ּ�
BOOL GetIPAddr(char *name, IN_ADDR *addr)
{
	HOSTENT *ptr = gethostbyname(name);
	if(ptr == NULL){
		err_display("gethostbyname()");
		return FALSE;
	}
	if(ptr->h_addrtype != AF_INET)
		return FALSE;
	GetInfo(ptr);

	memcpy(addr, ptr->h_addr, ptr->h_length);
	return TRUE;
}

// IPv4 �ּ� -> ������ �̸�
BOOL GetDomainName(IN_ADDR addr, char *name, int namelen)
{
	HOSTENT *ptr = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
	if(ptr == NULL){
		err_display("gethostbyaddr()");
		return FALSE;
	}
	if(ptr->h_addrtype != AF_INET)
		return FALSE;
	GetInfo(ptr);

	strncpy(name, ptr->h_name, namelen);
	return TRUE;
}

//��� ���� ���
void GetInfo(HOSTENT *ptr)
{
	printf("\n---- View All Information ----\n");
	printf("h_name : %s\n", ptr->h_name);
	int i = 0;
	while (ptr->h_aliases[i] != NULL)
	{
		printf("h_aliases[%d] : %s\n", i, ptr->h_aliases[i]);
		i++;
	}

	printf("h_addrtype : %d\n", ptr->h_addrtype);
	printf("h_length : %d\n", ptr->h_length);

	i = 0;
	while (ptr->h_addr_list[i] != NULL)
	{
		printf("h_addr_list[%d] : %s\n", i, ptr->h_addr_list[i]);
		i++;
	}
}

int main(int argc, char *argv[])
{
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	printf("������ �̸�(��ȯ ��) = %s\n", TESTNAME);

	// ������ �̸� -> IP �ּ�
	IN_ADDR addr;
	if(GetIPAddr(TESTNAME, &addr)){
		// �����̸� ��� ���
		printf("IP �ּ�(��ȯ ��) = %s\n", inet_ntoa(addr));
	
		// IP �ּ� -> ������ �̸�
		char name[256];
		char info[256];
		if(GetDomainName(addr, name, sizeof(name))){
			// �����̸� ��� ���
			printf("������ �̸�(�ٽ� ��ȯ ��) = %s\n", name);
		}
	}

	WSACleanup();
	return 0;
}