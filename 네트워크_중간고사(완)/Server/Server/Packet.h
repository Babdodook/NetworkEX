#pragma once
#include"basic.h"

//메뉴 프로토콜 패킹
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

//단순 성공 실패 결과와 메시지 패킹
void Packing(RESULTS result, char* buf, int *size, char* data)
{
	char *ptr = buf + sizeof(int);
	int strsize = strlen(data);

	memcpy(ptr, &result, sizeof(RESULTS));
	*size += sizeof(RESULTS);
	ptr += sizeof(RESULTS);

	memcpy(ptr, &strsize, sizeof(int));
	*size += sizeof(int);
	ptr += sizeof(int);

	memcpy(ptr, data, strsize);
	*size += strsize;

	ptr = buf;
	memcpy(ptr, size, sizeof(int));
}

//게임 결과와 메시지 패킹
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

//유저 정보 패킹
void Packing(PROTOCOL protocol, char* buf, int *size, _User user)
{
	char* ptr = buf + sizeof(int);
	int strsize = strlen(user.ID);

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

//유저 정보 언패킹
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

//정답 언패킹
void UnPacking(char* ptr, int *data)
{
	int strsize;

	memcpy(data, ptr, sizeof(int) * 3);
}