#pragma once

#include "Lomx.h"

#define Must 0x7473754D
#define Artx 0x78747241
#define Mnun 0x6E756E4D

typedef struct
{
	char name[52];
	unsigned int packedSize;	// 0x34
	unsigned int indexEnd;		// 0x38
	unsigned int indexStart;	// 0x3c
	unsigned int arrayOffset;	// 0x40
	// size = 0x44
} XMODULE_DATA;

typedef struct
{
	union
	{
		unsigned int buffer[980];
		struct
		{
			// must
			union
			{
				unsigned int must[644];
				struct
				{
					unsigned int signature;
					unsigned int unk1;
					unsigned int unk2;
					unsigned int unk3;
					XMODULE_DATA module[32];
				};
			};
			// artx
			unsigned int artx[132];
			// mnunx
			union
			{
				unsigned int mnun[204];
				struct
				{
					unsigned int signature;
					unsigned int unk1;
					unsigned int unk2;
					unsigned int unk3;
					unsigned int aOffset[1];
				};
			};
			// drid
			union
			{
				unsigned int drid[1024];
				struct
				{
					unsigned int aData[1][1024];
				};
			};
		};
	};
} XMAG;


BOOL check_module_name(__in LPSTR szName);
BOOL xmag_init(__in HANDLE hFile, __inout XMAG *pXmag);
BOOL load_xmodule(__in HANDLE hFile, __in LPCSTR lpcszModuleName, __in XMAG *xmag, __in XMODULE **ppXModule);
BOOL unload_xmodule(XMODULE *pXModule);