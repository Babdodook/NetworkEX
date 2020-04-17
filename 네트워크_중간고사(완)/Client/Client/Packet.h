#pragma once
#include"basic.h"

//���� ��������, ��Ʈ �ڷ��� ��ŷ
void Packing(GAME result, char*buf, int*size, int *data)
{
	char* ptr = buf + sizeof(int);

	memcpy(ptr, &result, sizeof(GAME));
	*size += sizeof(GAME);
	ptr += sizeof(GAME);

	memcpy(ptr, data, sizeof(int) * 3);
	ptr += sizeof(int) * 3;

	ptr = buf;
	memcpy(ptr, size, sizeof(int));
}

//���� ��������, ĳ���� �ڷ��� ��ŷ
void Packing(GAME result, char* buf, int *size, char* data)
{
	char *ptr = buf + sizeof(int);
	int strsize = strlen(data);

	memcpy(ptr, &result, sizeof(GAME));
	*size += sizeof(GAME);
	ptr += sizeof(GAME);

	memcpy(ptr, &strsize, sizeof(int));
	*size += sizeof(int);
	ptr += sizeof(int);

	memcpy(ptr, data, strsize);
	*size += strsize;

	ptr = buf;
	memcpy(ptr, size, sizeof(int));
}

//���� ���� ��ŷ
void Packing(PROTOCOL protocol, char *buf, int *size, _User user)
{
	int strsize = strlen(user.ID);
	char* ptr = buf+sizeof(int);

	memcpy(ptr, &protocol, sizeof(PROTOCOL));
	*size += sizeof(PROTOCOL);
	ptr += sizeof(PROTOCOL);

	memcpy(ptr, &strsize, sizeof(int));
	*size += sizeof(int);
	ptr += sizeof(int);

	memcpy(ptr, user.ID, strsize);
	*size += strsize;
	ptr += strsize;

	strsize = strlen(user.PW);
	memcpy(ptr, &strsize, sizeof(int));
	*size += sizeof(int);
	ptr += sizeof(int);

	memcpy(ptr, user.PW, strsize);
	*size += strsize;
	ptr += strsize;

	ptr = buf;
	memcpy(ptr, size, sizeof(int));	
}

//ĳ������ ��ŷ
void Packing(PROTOCOL protocol, char* buf, int *size, char* data)
{
	char *ptr = buf + sizeof(int);
	int strsize = strlen(data);

	memcpy(ptr, &protocol, sizeof(PROTOCOL));
	*size += sizeof(PROTOCOL);
	ptr += sizeof(PROTOCOL);

	memcpy(ptr, &strsize, sizeof(int));
	*size += sizeof(int);
	ptr += sizeof(int);

	memcpy(ptr, data, strsize);
	*size += strsize;

	ptr = buf;
	memcpy(ptr, size, sizeof(int));
}

//���� ���� ����ŷ
void UnPacking(char* ptr, _User *user)
{
	int strsize;

	memcpy(&strsize, ptr, sizeof(int));
	ptr += sizeof(int);

	memcpy(user->ID, ptr, strsize);
	ptr += strsize;
	user->ID[strsize] = '\0';

	memcpy(&strsize, ptr, sizeof(int));
	ptr += sizeof(int);

	memcpy(user->PW, ptr, strsize);
	user->PW[strsize] = '\0';
}

//�޽��� ����ŷ
void UnPacking(char* ptr, char *msg)
{
	int strsize;

	memcpy(&strsize, ptr, sizeof(int));
	ptr += sizeof(int);

	memcpy(msg, ptr, strsize);
	msg[strsize] = '\0';
}