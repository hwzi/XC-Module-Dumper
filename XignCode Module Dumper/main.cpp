#include "stdafx.h"

#include "lomx.h"
#include "xmag.h"

int main(int argc, char* argv[])
{
	CHAR *szPath = "C:\\Nexon\\MapleStory\\XignCode";
	CHAR *szPathOut;

	XMAG		xmag;
	HANDLE		hFile;
	XMODULE		*pXModule;
	CHAR		szFileName[MAX_PATH];

	if (argc > 1)
		szPath = argv[1];

	if (argc > 2)
		szPathOut = argv[2];
	else
		szPathOut = szPath;

	sprintf_s(szFileName, "%s\\xmag.xem", szPath);

	// set file attributes
	SetFileAttributesA(szFileName, FILE_ATTRIBUTE_NORMAL);

	// create xmag.xem file handle
	hFile = CreateFileA(szFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("CreateFile GLE = %08X\n", GetLastError());
		return -1;
	}
	
	// load xmag
	if (!xmag_init(hFile, &xmag))
	{
		printf("failed to load xmag\n");
		CloseHandle(hFile);
		return -1;
	}

	// enum modules
	XMODULE_DATA *xmodule = xmag.module;

	for (int i = 0; i < 32; i++)
	{
		if (check_module_name(xmodule->name))
		{
			printf("trying to extract %s\n", xmodule->name);
			if (!load_xmodule(hFile, xmodule->name, &xmag, &pXModule))
			{
				printf("\tfailed to load xmodule\n");
				continue;
			}

			sprintf_s(szFileName, "%s\\%s", szPathOut, xmodule->name);

			if (!extract_xmodule(pXModule, szFileName))
			{
				printf("\tfailed to extract\n");
			}

			// release mem
			unload_xmodule(pXModule);
		}

		xmodule++;
	}

	CloseHandle(hFile);

	return 0;
}

