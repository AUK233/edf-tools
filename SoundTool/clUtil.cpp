#include <Windows.h>
#include <string>
#include <format>

#include "clUtil.h"

int DirectoryExists(LPCSTR path)
{
	DWORD fileAttributes = GetFileAttributesA(path);
	if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
		return 0;
	}

	return (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

std::string EscapeControlChars(std::string sv) {
	std::string result;
	for (char c : sv) {
		switch (c) {
		case '\n': result += "\\n"; break;
		case '\t': result += "\\t"; break;
		case '\r': result += "\\r"; break;
		default:   result += c; break;
		}
	}
	return result;
}

std::string UnescapeControlChars(std::string sv)
{
	std::string result;
	for (size_t i = 0; i < sv.length(); ++i) {
		if (sv[i] == '\\' && i + 1 < sv.length()) {
			switch (sv[i + 1]) {
			case 'n': result += '\n'; ++i; break;
			case 't': result += '\t'; ++i; break;
			case 'r': result += '\r'; ++i; break;
			default:  result += sv[i]; break;
			}
		}
		else {
			result += sv[i];
		}
	}
	return result;
}

std::string RawDataToHexString(const char* data, size_t length)
{
	std::string out = "";
	for (int i = 0; i < length; i++) {
		out += std::format("{:02x}", data[i]);
	}
	return out;
}

UINT16 ReadUINT16LE(void const* pdata)
{
	return *(UINT16*)pdata;
}

INT32 ReadINT32LE(void const* pdata)
{
	return *(INT32*)pdata;
}

UINT16 ReadUINT16(void const* pdata, int isBigEndian)
{
	if (isBigEndian) {
		return _byteswap_ushort(*(UINT16*)pdata);
	}
	else {
		return *(UINT16*)pdata;
	}
}

UINT32 ReadUINT32(void const* pdata, int isBigEndian)
{
	if (isBigEndian) {
		return _byteswap_ulong(*(UINT32*)pdata);
	}
	else {
		return *(UINT32*)pdata;
	}
}

UINT64 ReadUINT64(void const* pdata, int isBigEndian)
{
	if (isBigEndian) {
		return _byteswap_ulong(*(UINT64*)pdata);
	}
	else {
		return *(UINT64*)pdata;
	}
}

void WriteUINT16LE(char* pdata, UINT16 value)
{
	*(UINT16*)pdata = value;
}

void WriteINT16LE(char* pdata, INT16 value)
{
	*(INT16*)pdata = value;
}

void WriteUINT32LE(char* pdata, UINT32 value)
{
	*(UINT32*)pdata = value;
}

void WriteINT32LE(char* pdata, INT32 value)
{
	*(INT32*)pdata = value;
}

void WriteINT16BE(char* pdata, INT16 value)
{
	*(INT16*)pdata = _byteswap_ushort(value);
}

void WriteINT32BE(char* pdata, INT32 value)
{
	*(INT32*)pdata = _byteswap_ulong(value);
}

void WriteINT64BE(char* pdata, INT64 value)
{
	*(INT64*)pdata = _byteswap_uint64(value);
}

void Swap4Bytes(char* pdata)
{
	UINT32* p = (UINT32*)pdata;
	*p = _byteswap_ulong(*p);
}
