#pragma once

int DirectoryExists(LPCSTR path);
std::string EscapeControlChars(std::string sv);
std::string UnescapeControlChars(std::string sv);
std::string RawDataToHexString(const char* data, size_t length);
std::vector<char> HexStringToRawData(const char* hexString);

UINT16 ReadUINT16LE(void const* pdata);
INT32 ReadINT32LE(void const* pdata);

UINT16 ReadUINT16(void const* pdata, int isBigEndian);
UINT32 ReadUINT32(void const* pdata, int isBigEndian);
UINT64 ReadUINT64(void const* pdata, int isBigEndian);

void WriteUINT16LE(char* pdata, UINT16 value);
void WriteINT16LE(char* pdata, INT16 value);
void WriteUINT32LE(char* pdata, UINT32 value);
void WriteINT32LE(char* pdata, INT32 value);

void WriteINT16BE(char* pdata, INT16 value);
void WriteINT32BE(char* pdata, INT32 value);
void WriteINT64BE(char* pdata, INT64 value);

void Swap4Bytes(char* pdata);