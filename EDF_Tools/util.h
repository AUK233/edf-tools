#pragma once

void Read2Bytes( unsigned char *chunk, std::vector<char> buf, int pos );
void Read2BytesReversed( unsigned char *chunk, std::vector<char> buf, int pos );
void Read4Bytes(unsigned char* chunk, std::vector<char> buf, int pos);
void Read4BytesReversed(unsigned char* chunk, std::vector<char> buf, int pos);
//Read specified number of bytes, which should be a multiple of 2
void ReadNBytesReversed(unsigned char* chunk, std::vector<char> buf, int pos, int num);

std::string ReadRaw(std::vector<char> buf, int pos, int num);
//wrong reading, do not use.
short ReadInt16(std::vector<char> buf, int pos, bool swapEndian = false);
uint16_t ReadUInt16(std::vector<char> buf, int pos, bool swapEndian = false);
//Poor performance as a float and should not be used
float ReadHalfFloat(std::vector<char> buf, int pos, bool swapEndian = false);

int GetIntFromChunk( unsigned char *chunk );
// Fast read of int32 using assembly
int __fastcall ReadInt32(uintptr_t pdata, int swapEndian = 0);
// Functions cannot be used to read float.

char* IntToBytes( int i, bool flip = true );

std::wstring ToString( int i );
std::wstring ToString( float f );

std::wstring ReadUnicode( std::vector<char> chunk, int pos, bool swapEndian = false );
std::string ReadASCII(std::vector<char> chunk, int pos);

//Util fn for simple tokenisation
std::wstring SimpleTokenise( std::wstring &input, wchar_t delim );

//Util fn for tokenising a wstring while retaining certain data
std::wstring Tokenise( std::wstring &input, const wchar_t delim[], wchar_t &firstDelim );

//Function to kill whitespace
std::wstring KillWhitespace( std::wstring in );
//Function to kill whitespace and throw error
std::wstring KillSpaceAndDebug(std::wstring in);

//Function to write a string to a char vector
void PushStringToVector(std::string strn, std::vector< char >* bytes);
//Function to write a string to a char vector, but no tail
void PushStringToVectorNoEnd(std::string strn, std::vector< char >* bytes);
//Function to write a wstring to a char vector
void PushWStringToVector( std::wstring strn, std::vector< char > *bytes );

//Checks if a string is a valid int
bool IsValidInt( const std::wstring input );
//Checks if a string is a valid float
bool IsValidFloat( const std::wstring input );

//Converts a int hex to float
float IntHexAsFloat(int in);

//Converts a string to lower case
std::wstring ConvertToLower( std::wstring in );

//Value type enum.
enum ValueType
{
	T_BOOL,
	T_STRING,
	T_FLOAT,
	T_INT,
	T_HEX,
	T_INVALID
};
//Function to determine "type" of string argument
ValueType DetermineType( std::wstring input );

//Helper fn to read a file
std::wstring ReadFile( const wchar_t* filename );

//Replaces all instances in a string
void FindAndReplaceAll( std::wstring & data, std::wstring toSearch, std::wstring replaceStr );
void FindAndReplaceAll(std::string& data, std::string toSearch, std::string replaceStr);

//Convert UTF8 to wide string
std::wstring UTF8ToWide(const std::string& source);
std::string WideToUTF8(const std::wstring& source);