#pragma once

#define Lomx 0x786D6F4C // Lomx

#pragma pack(push, 1)
typedef struct
{
	unsigned int signature;		// 0x00
	unsigned int size;			// 0x04
	unsigned int fileSize;		// 0x08
	unsigned int unkSize;		// 0x0C
	unsigned int dataSize;		// 0x10
	unsigned int fix;			// 0x14
	unsigned int unk3;			// 0x18
	unsigned int unk4;			// 0x1C
	unsigned char start[8];		// 0x20
	unsigned char data[5];		// 0x28
} XMODULE_HEADER;

typedef struct
{
	XMODULE_HEADER header;
	unsigned char data[1];			// 0x2D
} XMODULE;
#pragma pack(pop)

BOOL extract_xmodule(__in XMODULE *pXModule, __in LPCSTR lpcszFileName);