#pragma once

int DirectoryExists(LPCSTR path);
UINT16 ReadUINT16LE(void const* pdata);
INT32 ReadINT32LE(void const* pdata);

void WriteUINT16LE(char* pdata, UINT16 value);
void WriteINT32LE(char* pdata, INT32 value);
