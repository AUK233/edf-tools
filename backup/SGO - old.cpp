#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>

#include <iostream>
#include <locale>
#include <locale.h>
#include "util.h"
#include "SGO.h"

//Read data from SGO
void SGO::Read( std::wstring path )
{
	m_bVerbose = false;

	std::ifstream file( path, std::ios::binary | std::ios::ate );

	std::streamsize size = file.tellg( );
	file.seekg( 0, std::ios::beg );

	std::vector<char> buffer( size );
	if( file.read( buffer.data( ), size ) )
	{
		ParseFromData( buffer );
	}

	//Clear buffers
	buffer.clear( );
	file.close( );

	m_vecRawData.clear( );
	root.clear( );
}

void SGO::ParseFromData( std::vector< char > in )
{
	bool big_endian = false;
	int version = -1;

	m_vecRawData = in;

	//chunk buffer
	unsigned char seg[4];

	int position = 0;

	//Read 4 bytes.
	Read4Bytes( seg, m_vecRawData, position );

	if( seg[0] == 0x53 && seg[1] == 0x47 && seg[2] == 0x4f && seg[3] == 0x00 )
	{
		big_endian = true;
	}
	else if( seg[3] == 0x53 && seg[2] == 0x47 && seg[1] == 0x4f && seg[0] == 0x00 )
	{
		big_endian = false;
	}
	else
	{
		std::wcout << "Bad SGO file.\n";
		return;
	}

	//Grab file header information:
	position = 0x8;
	Read4Bytes( seg, m_vecRawData, position );
	version = GetIntFromChunk( seg );
	std::wcout << L"SGO Version: " + ToString( version ) + L"\n";

	//EDF 2017 p
	if( version == 0x10 )
	{
		position = 0x4;
		Read4Bytes( seg, m_vecRawData, position );
		m_iNumVariables = GetIntFromChunk( seg );

		std::wcout << L"Number of variables: " + ToString( m_iNumVariables ) + L"\n";

		position = 0xc;
		Read4Bytes( seg, m_vecRawData, position );
		int varNameTableStart = GetIntFromChunk( seg );

		position = 0x10;

		for( int i = 0; i < m_iNumVariables; ++i )
		{
			Read4Bytes( seg, m_vecRawData, varNameTableStart + i * 4 );
			int ofs = GetIntFromChunk( seg );

			std::wstring varName = ReadUnicode( m_vecRawData, varNameTableStart + i * 4 + ofs );
			if( m_bVerbose ) std::wcout << L"-Var(" + ToString( i ) + L") = " + varName + L"\n";

			root[varName] = ParseNode( position + ( i * ( 4 * 3 ) ) );
		}
	}

	//Parse();
}

SGONode * SGO::ParseNode( int position )
{
	unsigned char seg[4];

	//Read 'type' byte:
	Read4Bytes( seg, m_vecRawData, position );
	int type = GetIntFromChunk( seg );

	switch( type )
	{
		case 0x0: //Struct
		{
			//Read next chunk:
			Read4Bytes( seg, m_vecRawData, position + 4 );
			int size = GetIntFromChunk( seg ); //Value is largely useless. Worth keeping around, though.

			if( m_bVerbose ) std::wcout << L"--(STRUCT) Value is a structure of size" + ToString( size ) + L"\n";

			//Read string offset:
			Read4Bytes( seg, m_vecRawData, position + 8 );
			int structOfs = GetIntFromChunk( seg );

			return new SGONode( ParseStruct( position + structOfs, size ) );

			break;
		}
		case 0x1: //Int
		{
			//Read next chunk:
			Read4Bytes( seg, m_vecRawData, position + 4 );
			int size = GetIntFromChunk( seg ); //Value is largely useless. Worth keeping around, though.

			//Read int:
			Read4Bytes( seg, m_vecRawData, position + 8 );
			int val = GetIntFromChunk( seg );

			if( m_bVerbose ) std::wcout << L"--(FLOAT) Value is \"" + ToString( val ) + L"\"\n";

			return new SGONode( val );
			break;
		}
		case 0x2: //float
		{
			//Read next chunk:
			Read4Bytes( seg, m_vecRawData, position + 4 );
			int size = GetIntFromChunk( seg ); //Value is largely useless. Worth keeping around, though.

			//Read float:
			Read4BytesReversed( seg, m_vecRawData, position + 8 );
			float val;
			memcpy( &val, seg, sizeof( float ) );

			if( m_bVerbose ) std::wcout << L"--(FLOAT) Value is \"" + ToString( val ) + L"\"\n";

			return new SGONode( val );

			break;
		}
		case 0x3: //string
		{
			//Read next chunk:
			Read4Bytes( seg, m_vecRawData, position + 4 );
			int size = GetIntFromChunk( seg ); //Value is largely useless. Worth keeping around, though.

			//Read string offset:
			Read4Bytes( seg, m_vecRawData, position + 8 );
			int strnOfs = GetIntFromChunk( seg );

			std::wstring val = ReadUnicode( m_vecRawData, position + strnOfs );

			if( m_bVerbose ) std::wcout << L"--(STRING) Value is \"" + val + L"\"\n";

			return new SGONode( val );

			break;
		}
	}
}

std::vector< SGONode * > SGO::ParseStruct( int position, int size )
{
	std::vector< SGONode * > nodes;

	for( int i = 0; i < size; ++i )
	{
		nodes.push_back( ParseNode( position + ( i * ( 4 * 3 ) ) ));
	}

	return nodes;
};