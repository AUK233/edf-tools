#include <Windows.h>

#include "clUtil.h"

int DirectoryExists(LPCSTR path)
{
	DWORD fileAttributes = GetFileAttributesA(path);
	if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
		return 0;
	}

	return (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

UINT16 ReadUINT16LE(void const* pdata)
{
	return *(UINT16*)pdata;
}

INT32 ReadINT32LE(void const* pdata)
{
	return *(INT32*)pdata;
}
