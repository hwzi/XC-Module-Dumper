#include "stdafx.h"

#include "xmag.h"
#include "Lomx.h"

#pragma warning (disable:4018)

unsigned int get_hash(unsigned int *src, unsigned int len)
{
	unsigned int size = len >> 2;
	unsigned int result = 0;

	for (int i = 0; i < size; i++)
		result += src[i];

	return ~result;
}

BOOL check_module_name(__in LPSTR szName)
{
	LPSTR lpszTemp = szName;

	if (*lpszTemp == '\0')
		return FALSE;

	while (*lpszTemp)
	{
		if (!isascii(*lpszTemp))
			return FALSE;

		lpszTemp++;
	}

	return TRUE;
}

XMODULE_DATA *get_xmodule(__in LPCSTR lpcszName, XMAG *pXmag)
{
	XMODULE_DATA *axModule = pXmag->module;

	for (int i = 0; i < 32; i++)
	{
		if (strstr(axModule->name, lpcszName))
			return axModule;

		axModule++;
	}

	return nullptr;
}

BOOL load_xmodule(__in HANDLE hFile, __in LPCSTR lpcszModuleName, __in XMAG *xmag, __in XMODULE **ppXModule)
{
	DWORD adwOffset[1024];
	DWORD dwNumberOfBytesRead;
	DWORD dwRestOfSize;

	BYTE bBuffer[4096];

	LPBYTE pDest, pDestTemp;
	DWORD nIndex;

	XMODULE_DATA *xmodule = get_xmodule(lpcszModuleName, xmag);

	if (!xmodule)
		return FALSE;

	SetFilePointer(hFile, xmodule->arrayOffset << 12, NULL, FILE_BEGIN);
	ReadFile(hFile, adwOffset, 4096, &dwNumberOfBytesRead, NULL);

	if (xmodule->indexEnd > 0)
	{
		dwRestOfSize = xmodule->packedSize;
		nIndex = xmodule->indexStart;
		pDest = (LPBYTE)VirtualAlloc(NULL, xmodule->packedSize, MEM_RESERVE, PAGE_READWRITE);

		if (!pDest)
			return FALSE;

		VirtualAlloc(pDest, xmodule->packedSize, MEM_COMMIT, PAGE_READWRITE);
		pDestTemp = pDest;

		for (int i = 0; i < xmodule->indexEnd; i++)
		{
			SetFilePointer(hFile, adwOffset[nIndex] << 12, NULL, FILE_BEGIN);
			ReadFile(hFile, bBuffer, 4096, &dwNumberOfBytesRead, NULL);

			if (dwRestOfSize < 4096)
				memcpy(pDestTemp, bBuffer, dwRestOfSize);
			else
			{
				memcpy(pDestTemp, bBuffer, 4096);
				dwRestOfSize -= 4096;
				pDestTemp += 4096;
				nIndex = (nIndex + 1) & 0x3FF;
			}
		}

		*ppXModule = (XMODULE*)pDest;

		return TRUE;
	}

	return FALSE;
}

BOOL unload_xmodule(__in XMODULE *pXModule)
{
	return VirtualFree(pXModule, 0, MEM_RELEASE);
}

BOOL xmag_init(__in HANDLE hFile, __inout XMAG *pXmag)
{
	DWORD dwNumberOfBytesRead;
	DWORD dwXorSource;

	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
	if (!ReadFile(hFile, pXmag->buffer, 0xA10 + 0x210 + 0x330, &dwNumberOfBytesRead, NULL))
	{
		printf("ReadFile GLE = %08X\n", GetLastError());

		return FALSE;
	}

	dwXorSource = Must;

	for (int i = 0; i < 644 + 132 + 204; i++)
		pXmag->buffer[i] ^= dwXorSource++;

	// must check
	if (pXmag->must[0] != Must || get_hash(pXmag->must, 0xa10) != 0)
	{
		printf("Must is broken??\n");

		return FALSE;
	}

	// artx check
	if (pXmag->artx[132 - 1] != Artx)
	{
		printf("Artx is broken??\n");
		pXmag->artx[132 - 1] = Artx;
		pXmag->artx[132 - 2] = 1;
		pXmag->artx[132 - 3] = get_hash(pXmag->artx, 0x210);
		pXmag->artx[132 - 4] = 0;
	}

	// mnun check
	if (pXmag->mnun[0] != Mnun || pXmag->mnun[1] != 1 || get_hash(pXmag->mnun, 0x330) != 0)
	{
		printf("Mnun is broken??\n");
		memset(pXmag->mnun, 0, 0x330);
		pXmag->mnun[0] = Mnun;
		pXmag->mnun[1] = 1;
		pXmag->mnun[2] = 0;

		// fuck this
		printf("cbf to add\n");

		pXmag->drid[2] = get_hash(pXmag->drid, 4096);

		return FALSE;
	}
	else
	{
		for (int i = 0; i < 1; i++)
		{
			SetFilePointer(hFile, pXmag->aOffset[i] << 12, NULL, FILE_BEGIN);
			if (!ReadFile(hFile, pXmag->aData[i], 4096, &dwNumberOfBytesRead, NULL))
			{
				printf("ReadFile GLE = %08X\n", GetLastError());

				return FALSE;
			}
		}
	}

	// check?
	if (pXmag->must[0] != Must || pXmag->must[2] >= 0x00014801 || pXmag->must[3] >= 0x00014801)
	{
		printf("Must is broken??\n");

		return FALSE;
	}

	return TRUE;
}