#include "stdafx.h"
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <codecvt>
#include <windows.h>
#include "util.h"
#include "include/half.hpp"

void Read2Bytes( unsigned char *chunk, std::vector<char> buf, int pos )
{
	chunk[0] = buf[ pos + 1];
	chunk[1] = buf[ pos ];
}

void Read2BytesReversed( unsigned char *chunk, std::vector<char> buf, int pos )
{
	chunk[0] = buf[ pos ];
	chunk[1] = buf[ pos + 1 ];
}

void Read4Bytes(unsigned char* chunk, std::vector<char> buf, int pos)
{
	/*if( globals->endianMode )
	{
		Read4BytesReversed( chunk, buf, pos );
		return;
	}*/

	chunk[3] = buf[pos];
	chunk[2] = buf[pos + 1];
	chunk[1] = buf[pos + 2];
	chunk[0] = buf[pos + 3];
}

void Read4BytesReversed(unsigned char* chunk, std::vector<char> buf, int pos)
{
	chunk[0] = buf[pos];
	chunk[1] = buf[pos + 1];
	chunk[2] = buf[pos + 2];
	chunk[3] = buf[pos + 3];
}

//Read specified number of bytes, which should be a multiple of 2
void ReadNBytesReversed(unsigned char* chunk, std::vector<char> buf, int pos, int num)
{
	for (int i = num - 1; i >= 0; --i)
	{
		chunk[i] = buf[pos + i];
	}
}

std::string ReadRaw(std::vector<char> buf, int pos, int num)
{
	std::string str="";
	unsigned char chunk;
	char tempbuffer[3];

	for (int i = 0; i < num; i++)
	{
		chunk = buf[pos + i];
		if (chunk < 0x10)
			str += "0";

		str += itoa(chunk, tempbuffer, 16);
	}
	return str;
}

//wrong reading, do not use.
short ReadInt16(std::vector<char> buf, int pos, bool swapEndian)
{
	char chunk[2];
	if (swapEndian)
	{
		chunk[0] = buf[pos];
		chunk[1] = buf[pos + 1];
	}
	else
	{
		chunk[0] = buf[pos + 1];
		chunk[1] = buf[pos];
	}

	short num = 0;
	for (int i = 0; i < 2; i++)
	{
		num <<= 8;
		num |= chunk[i];
	}

	return num;
}

uint16_t ReadUInt16(std::vector<char> buf, int pos, bool swapEndian)
{
	char chunk[2];
	if (swapEndian)
	{
		chunk[0] = buf[pos];
		chunk[1] = buf[pos + 1];
	}
	else
	{
		chunk[0] = buf[pos + 1];
		chunk[1] = buf[pos];
	}

	uint16_t num = 0;
	for (int i = 0; i < 2; i++)
	{
		num <<= 8;
		num |= chunk[i];
	}

	return num;
}

float ReadHalfFloat(std::vector<char> buf, int pos, bool swapEndian)
{
	char chunk[2];
	if (swapEndian)
	{
		chunk[0] = buf[pos + 1];
		chunk[1] = buf[pos];
	}
	else
	{
		chunk[0] = buf[pos];
		chunk[1] = buf[pos + 1];
	}

	half_float::half hf;

	memcpy(&hf, &chunk, sizeof(hf));

	float f = float(hf);
	
	return f;
}

int GetIntFromChunk( unsigned char *chunk )
{
	int num = 0;
	for (int i = 0; i < 4; i++)
	{
		num <<= 8;
		num |= chunk[i];
	}

	return num;
}

char* IntToBytes( int i, bool flip )
{
	char *bytes = (char*)malloc( sizeof( char ) * 4 );
	unsigned long n = i;

	if( !flip )
	{
		bytes[0] = ( n >> 24 ) & 0xFF;
		bytes[1] = ( n >> 16 ) & 0xFF;
		bytes[2] = ( n >> 8 ) & 0xFF;
		bytes[3] = n & 0xFF;
	}
	else
	{
		bytes[0] = n & 0xFF;
		bytes[1] = (n >> 8) & 0xFF;
		bytes[2] = (n >> 16) & 0xFF;
		bytes[3] = (n >> 24) & 0xFF;
	}

	return bytes;
}

std::wstring ReadUnicode( std::vector<char> chunk, int pos, bool swapEndian )
{
	if( pos > chunk.size( ) )
		return L"";

	unsigned int bufPos = pos;

	std::vector< unsigned char > bytes;

	int zeroCount = 0;

	//Repeat until EOF, or otherwise broken
	while( bufPos < chunk.size() )
	{
		if( swapEndian )
		{
			if( bufPos % 2 )
			{
				bytes.push_back( chunk[bufPos] );
				bytes.push_back( chunk[bufPos-1] );
			}
		}
		else
			bytes.push_back( chunk[bufPos] );

		if( chunk[bufPos] == 0x00 )
		{
			zeroCount++;
			if( zeroCount == 2 )
				break;
		}
		else
			zeroCount = 0;
		bufPos++;
	}

	//if( bytes.size( ) == 0 );
	//return L"";


	std::wstring wstr(reinterpret_cast<wchar_t*>(&bytes[0]), bytes.size()/sizeof(wchar_t));

	return wstr;
}

std::string ReadASCII(std::vector<char> chunk, int pos)
{
	if (pos > chunk.size())
		return "";

	unsigned int bufPos = pos;

	std::vector< unsigned char > bytes;

	int zeroCount = 0;

	//Repeat until EOF, or otherwise broken
	while (bufPos < chunk.size())
	{
		bytes.push_back(chunk[bufPos]);

		if (chunk[bufPos] == 0x00)
		{
			zeroCount++;
			if (zeroCount == 1)
				break;
		}
		else
			zeroCount = 0;
		bufPos++;
	}

	std::string str(reinterpret_cast<char*>(&bytes[0]), bytes.size() / sizeof(char));

	return str;
}

std::wstring ToString( int i )
{
	return std::to_wstring( (uint64_t) i );
}
std::wstring ToString( float f )
{
	return std::to_wstring( (long double) f );
}

//Util fn for simple tokenisation
std::wstring SimpleTokenise( std::wstring &input, wchar_t delim )
{
	for( int pos = 0; pos < input.size( ); pos++ )
	{
		if( input[pos] == delim )
		{
			//Get left side:
			std::wstring out = input.c_str( );
			out.erase( pos, out.size( ) - pos );
			input.erase( 0, pos + 1 );
			return out;
		}
	}
	std::wstring out = input.c_str( );
	input.clear( );
	return out;
}

//Util fn for tokenising a wstring while retaining certain data
std::wstring Tokenise( std::wstring &input, const wchar_t delim[], wchar_t &firstDelim )
{
	for( int pos = 0; pos < input.size( ); pos++ )
	{
		for( int i = 0; i < std::wcslen( delim ); i++ )
		{
			if( input[pos] == delim[i] )
			{
				firstDelim = input[pos];

				//Get left side:
				std::wstring out = input.c_str( );
				out.erase( pos, out.size( ) - pos );
				input.erase( 0, pos + 1 );

				return out;
			}
		}
	}
	std::wstring out = input.c_str( );
	input.clear( );
	return out;
}

//Function to kill whitespace
std::wstring KillWhitespace( std::wstring in )
{
	bool isInQuote = false;

	std::wstring out = in;

	for( int i = in.size( ); i >= 0; i-- )
	{
		if( ( out[i] == L' ' || out[i] == L'\t' ) && !isInQuote )
		{
			out.erase( i, 1 );
			out.shrink_to_fit( );
			continue;
		}
		if( out[i] == L'\"' )
		{
			isInQuote = !isInQuote;
		}
	}

	return out;
}

//Function to kill whitespace and throw error, now it has a problem with judgment
std::wstring KillSpaceAndDebug(std::wstring in)
{
	bool isInQuote = false;

	std::wstring out = in;
	wchar_t delim[13] = { L'+', L'-', L'*', L'/', L'(', L')', L';', L'=', L'>', L'<', L' ', L'\t', L'\n' };

	for (int i = in.size(); i >= 0; i--)
	{
		if ((out[i] == L' ' || out[i] == L'\t') && !isInQuote )
		{
			if (out[i - 1] != delim[12] && out[i + 1] != delim[12])
			{
				std::wcout << "\nMissing \",\" between 2 values:\n";
				std::wcout << out;
				std::wcout << "\n";
				system("pause");
				exit(0);
			}
			out.erase(i, 1);
			out.shrink_to_fit();
			continue;
		}

		if (out[i] == L'\"')
		{
			isInQuote = !isInQuote;
		}
	}

	return out;
}

//Checks if a string is a valid int
bool IsValidInt( const std::wstring input )
{
	int num;
	bool valid = true;
	try
	{
		num = stoi( input );
	}
	catch( const std::exception& e )
	{
		valid = false;
	}

	return valid;
}

//Checks if a string is a valid float
bool IsValidFloat( const std::wstring input )
{
	float num;
	bool valid = true;
	try
	{
		num = stof( input );
	}
	catch( const std::exception& e )
	{
		valid = false;
	}

	return valid;
}

//Converts a string to lower case
std::wstring ConvertToLower( std::wstring in )
{
	std::wstring out;
	for( int i = 0; i < in.size( ); ++i )
	{
		out += towlower( in[i] );
	}
	return out;
}

//Function to determine "type" of string argument
ValueType DetermineType( std::wstring input )
{
	std::wstring lower = ConvertToLower( input );
	if( lower == L"true" || lower == L"false" )
	{
		return T_BOOL;
	}
	if( input[0] == L'\"' && input.back( ) == L'"' )
		return T_STRING;

	if( lower.back( ) == L'f' || input.find( L'.' ) != std::wstring::npos )
	{
		if( IsValidFloat( input ) )
			return T_FLOAT;
		else
			return T_INVALID;
	}

	if( lower[1] == L'x' || lower.back( ) == L'h' )
	{
		return T_HEX;
	}

	if( IsValidInt( input ) )
		return T_INT;

	return T_INVALID;
}

///Helper fn to read a file
std::wstring ReadFile( const wchar_t* filename )
{
	std::wifstream wif( filename, std::ios::binary );
	//wif.imbue( std::locale( std::locale::empty( ), new std::codecvt_utf8<wchar_t> ) );

	const unsigned long MaxCode = 0x10ffff;
	const std::codecvt_mode Mode = std::generate_header;
	std::locale utf16_locale( wif.getloc( ), new std::codecvt_utf16<wchar_t, MaxCode, Mode> );

	wif.imbue( utf16_locale );

	std::wstringstream wss;
	wss << wif.rdbuf( );
	return wss.str( );
}

///Replaces all instances in a string
void FindAndReplaceAll( std::wstring & data, std::wstring toSearch, std::wstring replaceStr )
{
	// Get the first occurrence
	size_t pos = data.find( toSearch );

	// Repeat till end is reached
	while( pos != std::wstring::npos )
	{
		// Replace this occurrence of Sub String
		data.replace( pos, toSearch.size( ), replaceStr );
		// Get the next occurrence from the current position
		pos = data.find( toSearch, pos + replaceStr.size( ) );
	}
}

void FindAndReplaceAll(std::string& data, std::string toSearch, std::string replaceStr)
{
	// Get the first occurrence
	size_t pos = data.find(toSearch);

	// Repeat till end is reached
	while (pos != std::string::npos)
	{
		// Replace this occurrence of Sub String
		data.replace(pos, toSearch.size(), replaceStr);
		// Get the next occurrence from the current position
		pos = data.find(toSearch, pos + replaceStr.size());
	}
}

//Function to write a string to a char vector
void PushStringToVector(std::string strn, std::vector< char >* bytes)
{
	for (int i = 0; i < strn.size(); i++)
	{
		bytes->push_back(strn[i]);
	}
	//Zero terminate
	bytes->push_back(0x0);
}

//Function to write a string to a char vector, but no tail
void PushStringToVectorNoEnd(std::string strn, std::vector< char >* bytes)
{
	for (int i = 0; i < strn.size(); i++)
	{
		bytes->push_back(strn[i]);
	}
}

///Function to write a wstring to a char vector
void PushWStringToVector( std::wstring strn, std::vector< char > *bytes )
{
	char* strnBytes;
	strnBytes = reinterpret_cast<char*>( &strn[0] );

	int size = strn.size( ) * sizeof( strn.front( ) );

	for( int i = 0; i < size; i++ )
	{
		bytes->push_back( strnBytes[i] );
	}
	//Zero terminate
	bytes->push_back( 0x0 );
	bytes->push_back( 0x0 );
}

//Convert UTF8 to wide string
std::wstring UTF8ToWide(const std::string& source)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	return conv.from_bytes(source);
}

std::string WideToUTF8(const std::wstring& source)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	return conv.to_bytes(source);
}
