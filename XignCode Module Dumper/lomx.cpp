#include "stdafx.h"

#include "lomx.h"
#include "unsp.h"

typedef struct
{
	unsigned int tre;
	unsigned int allocsz;
	unsigned int firstbyte;
	unsigned short *table;
	unsigned int tablesz;
} unsp_ctx;

int unsp_init(unsp_ctx *ctx, unsigned char c)
{
	unsigned char v;

	if (c >= 0xE1)
		return 4;

	v = c / 9;
	ctx->tre = c % 9;
	ctx->allocsz = v % 5;
	ctx->firstbyte = v / 5;

	// init table
	ctx->tablesz = ((0x300 << ((ctx->tre + ctx->allocsz) & 0xff)) + 0x736) * sizeof(unsigned short);
	ctx->table = (unsigned short *)malloc(ctx->tablesz);

	if (!ctx->table)
		return 1;

	return 0;
}

void unsp_clear(unsp_ctx *ctx)
{
	if (ctx->table)
		free(ctx->table);
}

int unsp_unpack(unsp_ctx *ctx, const char *src, unsigned int ssize, char *dst, unsigned int dsize)
{
	return very_real_unpack(ctx->table, ctx->tablesz, ctx->tre, ctx->allocsz, ctx->firstbyte, src, ssize, dst, dsize);
}

bool unspack(unsigned char c, void *src, unsigned int ssize, void *dst, unsigned int dsize)
{
	unsp_ctx ctx;

	if (unsp_init(&ctx, c) != 0)
		return false;

	unsp_unpack(&ctx, (const char *)src, ssize, (char *)dst, dsize);

	unsp_clear(&ctx);

	return true;
}

void update_unsp_src_sub(unsigned int offset, unsigned char *dst, unsigned int size, unsigned short *src, unsigned int mod)
{
	unsigned char	temp;
	unsigned int	swap_offset;
	unsigned short	data = src[offset % mod];

	switch (data & 0xF000)
	{
		case 0x1000:
		{
			// swap
			swap_offset = ((data & 0x0FFF) % size) + offset;
			if (swap_offset < size)
			{
				temp = dst[swap_offset];
				dst[swap_offset] = dst[offset];
				dst[offset] = temp;
			}
			break;
		}
		case 0x2000:
		{
			// xor
			dst[offset] ^= data;
			break;
		}
		default:
		{
			break;
		}
	}
}

void update_unsp_src(void *dst, unsigned int size, void *src, unsigned int mod)
{
	for (int offset = size - 1; offset >= 0; offset--)
	{
		update_unsp_src_sub(offset, (unsigned char *)dst, size, (unsigned short *)src, mod);
	}
}

BOOL unspack_xmodule(XMODULE *pXModule, void *src, unsigned int ssize, void *dst, unsigned int dsize)
{
	void *src_local = VirtualAlloc(NULL, ssize, MEM_RESERVE, PAGE_READWRITE);
	if (!src_local)
		return false;

	VirtualAlloc(src_local, ssize, MEM_COMMIT, PAGE_READWRITE);
	memcpy(src_local, src, ssize);

	update_unsp_src(src_local, ssize, pXModule->header.start, 2);
	update_unsp_src(src_local, ssize, pXModule->header.start, 4);
	
	// sub_1003BE99
	unspack(pXModule->header.data[0], src_local, ssize, dst, dsize);

	return VirtualFree(src_local, 0, MEM_RELEASE);
}

BOOL extract_xmodule(__in XMODULE *pXModule, __in LPCSTR lpcszFileName)
{
	HANDLE hFile;
	DWORD dwFileSize;
	DWORD dwNumberOfBytesWritten;
	XMODULE *pFile;

	if (pXModule->header.signature != Lomx)
		return false;

	dwFileSize = pXModule->header.fileSize + pXModule->header.unkSize + sizeof(XMODULE_HEADER);

	pFile = (XMODULE *)VirtualAlloc(NULL, dwFileSize, MEM_RESERVE, PAGE_READWRITE);

	if (!pFile)
		return false;

	if (pFile)
	{
		VirtualAlloc(pFile, dwFileSize, MEM_COMMIT, PAGE_READWRITE);
		memset(pFile, 0, dwFileSize);
	}

	// copy member
	pFile->header = pXModule->header;

	if (unspack_xmodule(pXModule, pXModule->data, pXModule->header.dataSize, pFile->data, pXModule->header.fileSize + pXModule->header.unkSize))
	{
		IMAGE_NT_HEADERS *pImageNtHeader = (IMAGE_NT_HEADERS *)pFile->data;
		IMAGE_SECTION_HEADER *aImageSectionHeader;

		if (pImageNtHeader->Signature != IMAGE_NT_SIGNATURE)
		{
			VirtualFree(pFile, 0, MEM_RELEASE);
			return FALSE;
		}
		// dll
		CHAR szFileName[MAX_PATH];
		sprintf_s(szFileName, "%s.dll", lpcszFileName);

		hFile = CreateFileA(szFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			// DOS HEADER!
#pragma region dos
			IMAGE_DOS_HEADER imageDosHeader;
			imageDosHeader.e_magic = IMAGE_DOS_SIGNATURE;
			imageDosHeader.e_cblp = 0x90;
			imageDosHeader.e_cp = 3;
			imageDosHeader.e_crlc = 0;
			imageDosHeader.e_cparhdr = 4;
			imageDosHeader.e_minalloc = 0;
			imageDosHeader.e_maxalloc = -1;
			imageDosHeader.e_ss = 0;
			imageDosHeader.e_sp = 0xB8;
			imageDosHeader.e_csum = 0;
			imageDosHeader.e_ip = 0;
			imageDosHeader.e_cs = 0;
			imageDosHeader.e_lfarlc = 0x40;
			imageDosHeader.e_ovno = 0;
			imageDosHeader.e_res[0] = 0;
			imageDosHeader.e_res[1] = 0;
			imageDosHeader.e_res[2] = 0;
			imageDosHeader.e_res[3] = 0;
			imageDosHeader.e_oemid = 0;
			imageDosHeader.e_oeminfo = 0;
			imageDosHeader.e_res2[0] = 0;
			imageDosHeader.e_res2[1] = 0;
			imageDosHeader.e_res2[2] = 0;
			imageDosHeader.e_res2[3] = 0;
			imageDosHeader.e_res2[4] = 0;
			imageDosHeader.e_res2[5] = 0;
			imageDosHeader.e_res2[6] = 0;
			imageDosHeader.e_res2[7] = 0;
			imageDosHeader.e_res2[8] = 0;
			imageDosHeader.e_res2[9] = 0;
			imageDosHeader.e_lfanew = 0xE0;

			SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
			WriteFile(hFile, &imageDosHeader, sizeof(IMAGE_DOS_HEADER), &dwNumberOfBytesWritten, NULL);
#pragma endregion write dos header to file

			// write nt header
			SetFilePointer(hFile, 0xE0, NULL, FILE_BEGIN);
			WriteFile(hFile, pFile->data, pXModule->header.size - sizeof(XMODULE_HEADER), &dwNumberOfBytesWritten, NULL);

			// write section
			aImageSectionHeader = reinterpret_cast<IMAGE_SECTION_HEADER*>(reinterpret_cast<PBYTE>(pImageNtHeader)+sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + pImageNtHeader->FileHeader.SizeOfOptionalHeader);

			for (int i = 0; i < pImageNtHeader->FileHeader.NumberOfSections; i++)
			{
				SetFilePointer(hFile, aImageSectionHeader[i].PointerToRawData, NULL, FILE_BEGIN);
				WriteFile(hFile, ((PBYTE)pFile) + pXModule->header.fix + aImageSectionHeader[i].PointerToRawData, aImageSectionHeader[i].SizeOfRawData, &dwNumberOfBytesWritten, NULL);
			}
			CloseHandle(hFile);
		}
		else
			_tprintf(_T("CreateFile GLE : %08X\n"), GetLastError());

		VirtualFree(pFile, 0, MEM_RELEASE);

		return TRUE;
	}

	VirtualFree(pFile, 0, MEM_RELEASE);

	return FALSE;
}