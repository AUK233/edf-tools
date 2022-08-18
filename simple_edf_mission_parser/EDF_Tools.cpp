// simple_edf_mission_parser.cpp : Defines the entry point for the console application.
//

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

#ifndef MS_STDLIB_BUGS
#  if ( _MSC_VER || __MINGW32__ || __MSVCRT__ )
#    define MS_STDLIB_BUGS 1
#  else
#    define MS_STDLIB_BUGS 0
#  endif
#endif

#if MS_STDLIB_BUGS
#  include <io.h>
#  include <fcntl.h>
#endif

void init_locale(void)
{
#if MS_STDLIB_BUGS
  char cp_utf16le[] = ".1200";
  setlocale( LC_ALL, cp_utf16le );
  _setmode( _fileno(stdout), _O_WTEXT );
#else
  // The correct locale name may vary by OS, e.g., "en_US.utf8".
  constexpr char locale_name[] = "";
  setlocale( LC_ALL, locale_name );
  std::locale::global(std::locale(locale_name));
  std::wcin.imbue(std::locale())
  std::wcout.imbue(std::locale());
#endif
}


#include "util.h"
#include "JSONAMLParser.h"
#include "MissionScript.h" //TODO: Implement mission script class that stores and proccess data
#include "RMPA.h" //TODO: Implement RMPA class that stores and proccess data
#include "RAB.h" //RAB extractor
#include "SGO.h" //SGO parser.

#include "MDB.h" //MDB parser.

#include "ModManager.h"

#define FLAG_VERBOSE 1
#define FLAG_CREATE_FOLDER 2

//Keep this here for now
//#define TOOL_RABARCHIVER 1

void ProcessFile( const std::wstring& path, int extraFlags )
{
    using namespace std;

	//Get file extension:
	size_t lastindex = path.find_last_of( L"." );

	if( lastindex != wstring::npos )
	{ 
		wstring extension = path.substr( lastindex + 1, extension.size( ) - lastindex );
		wstring strn = path.substr( 0, lastindex );

		//List all files
		if( strn == L"*" )
		{
			wstring directory;

			const size_t last_slash_idx = path.rfind( L'\\' );
			if( std::string::npos != last_slash_idx )
			{
				directory = path.substr( 0, last_slash_idx );
			}

			if( directory.size( ) == 0 )
			{
				wchar_t CurDirectory[512];
				GetCurrentDirectoryW( 512, CurDirectory );

				directory = CurDirectory;
			}
			size_t directorySize = directory.size( );
			directory += L"\\*." + extension;

			struct _wfinddata_t dirFile;
			long hFile;
			if( ( hFile = _wfindfirst( directory.c_str(), &dirFile ) ) != -1 )
			{
				do
				{
					if( !lstrcmpW( dirFile.name, L"." ) ) continue;
					if( !lstrcmpW( dirFile.name, L".." ) ) continue;

					//wcout << dirFile.name;
					//wcout << L"\n";

					directory = directory.substr( 0, directorySize );

					ProcessFile( directory + L"\\" + dirFile.name, extraFlags | FLAG_CREATE_FOLDER );

				} while( _wfindnext( hFile, &dirFile ) == 0 );
				_findclose( hFile );
			}
			return;
		}

		//Convert extension to lowercase for proccessing:
		extension = ConvertToLower( extension );

		if( extension == L"rmpa" )
		{
            unique_ptr<CRMPA> rmpa = make_unique<CRMPA>();
			rmpa->Read( strn );
		}
		else if( extension == L"txt" )
		{
            unique_ptr<CMissionScript> script = make_unique<CMissionScript>();
			script->Write( strn, extraFlags );
			script.reset( );
		}
		else if( extension == L"bvm" )
		{
            unique_ptr<CMissionScript> script = make_unique<CMissionScript>();
			script->LoadLanguage( L"EDF5_2C_MissionCommands.jsonaml", 0 );
			script->LoadLanguage( L"EDF5_2D_MissionCommands.jsonaml", 1 );
			script->Read( strn );
			script.reset( );
		}
		else if( extension == L"rab" )
		{
			std::unique_ptr< RAB > rabReader = std::make_unique< RAB >( );
			rabReader->Read( strn, false );
			rabReader.reset( );
		}
		else if( extension == L"mrab" )
		{
			std::unique_ptr< RAB > rabReader = std::make_unique< RAB >( );
			rabReader->Read( strn, true );
			rabReader.reset( );
		}
		else if (extension == L"mdb")
		{
			wstring scstr;
			wcout << L"Single Core? (0 is false, 1 is true) : ";
			wcin >> scstr;
			bool onecore = false;
			if (stoi(scstr) == 1)
			{
				onecore = true;
				wcout << L"\nWill now use a single core to read the file!";
			}
			wcout << L"\n\n";

			unique_ptr<CMDBtoXML> script = make_unique<CMDBtoXML>();
			script->Read(strn, onecore);
			script.reset();
		}
		else if (extension == L"xml")
		{
			size_t xmlIndex = strn.find_last_of(L"_");
			wstring xmlExtension = strn.substr(xmlIndex + 1, xmlExtension.size() - xmlIndex);
			wstring xmlStrn = strn.substr(0, xmlIndex);

			xmlExtension = ConvertToLower(xmlExtension);
			// To MDB File, no need for multi-core.
			if (xmlExtension == L"mdb")
			{
				unique_ptr< CXMLToMDB > script = make_unique< CXMLToMDB >();
				script->Write(xmlStrn, false);
				script.reset();
			}
		}
	}
	else
	{
		//Search for valid files:


	}
}

//UNDONE: Expression parsing is not within project scope at this time
#if 0
struct ExprContext
{
	ExprContext( std::wstring strn, int l )
	{
		rawStrn = strn;
		level = l;
	}
	std::wstring rawStrn;
	int level;
};
struct ExprParser
{
	ExprParser( std::wstring in )
	{
		originalStrn = in.c_str( );
		dbgOutput = in + L'\n';

		//Extract brackets
		std::vector < std::wstring > bracketedStrings;
		int level = 1;

		std::wstring preBracketString;
		std::wstring bracket = ExtractBrackets( in, preBracketString );

		if( preBracketString.size( ) > 0 )
		{
			expression.push_back( ExprContext( preBracketString, 0 ) );
			in.erase( 0, preBracketString.size( ) );
			preBracketString.clear( );
		}

		while( bracket.size( ) > 0 )
		{
			bracketedStrings.push_back( bracket );
			level++;
			bracket = ExtractBrackets( bracketedStrings.back( ), preBracketString );
			expression.push_back( ExprContext( preBracketString, level - 1 ) );
			preBracketString.clear( );
			

			if( bracket.size( ) == 0 )
			{
				level = 1;
				bracket = ExtractBrackets( in, preBracketString );
				if( preBracketString.size( ) > 0 )
				{
					expression.push_back( ExprContext( preBracketString, 0 ) );
					in.erase( 0, preBracketString.size( ) );
					preBracketString.clear( );
				}
			}
		}

		//simple sorting algorthm:
		for( int i = 0; i < expression.size( ); ++i )
		{
			for( int j = 1; j < expression.size( ); ++j )
			{
				if( expression[j].level < expression[j - 1].level )
				{
					std::swap( expression[j], expression[j - 1] );
				}
			}
		}

		//Print proccessed string

		dbgOutput += L"STAGE 1:\n";
		for( int i = 0; i < expression.size( ); ++i )
		{
			//extract brackets
			dbgOutput += L"Level: " + ToString(expression[i].level) + L" string: " + expression[i].rawStrn + L'\n';
		}
	}

	std::wstring ExtractBrackets( std::wstring &in, std::wstring &preBracketString )
	{
		std::wstring retn;
		int depth = 0;
		int startPos = 0;
		for( int i = 0; i < in.size( ); ++i )
		{
			if( in[i] == L'(' )
			{
				++depth;
				if( depth == 1 )
				{
					startPos = i;
					continue;
				}
			}
			else if( in[i] == L')' )
			{
				--depth;
				if( depth == 0 )
				{
					in.erase( startPos, i - startPos + 1 );
					return retn;
				}
			}
			if( depth != 0 )
			{
				retn.push_back( in[i] );
			}
			else
				preBracketString.push_back( in[i] );
		}

		return retn;
	}
	std::wstring ExtractBrackets( std::wstring &in )
	{
		std::wstring retn;
		int depth = 0;
		int startPos = 0;
		for( int i = 0; i < in.size( ); ++i )
		{
			if( in[i] == L'(' )
			{
				++depth;
				if( depth == 1 )
				{
					startPos = i;
					continue;
				}
			}
			else if( in[i] == L')' )
			{
				--depth;
				if( depth == 0 )
				{
					in.erase( startPos, i - startPos + 1 );
					return retn;
				}
			}
			if( depth != 0 )
			{
				retn.push_back( in[i] );
			}
		}

		return retn;
	}

	std::wstring originalStrn;
	std::wstring dbgOutput;

	std::vector< ExprContext > expression;
};

std::wstring TestProccess( std::wstring input )
{
	//first thing first: Kill whitespace:
	std::wstring proccessStrn = KillWhitespace( input );

	ExprParser parser( proccessStrn );

	return parser.dbgOutput;
}
#endif

void TestReadMab( )
{
	using namespace std;

	std::ifstream file( L"edf41_2.mab", std::ios::binary | std::ios::ate );

	std::streamsize size = file.tellg( );
	file.seekg( 0, std::ios::beg );

	std::vector<char> buffer( size );
	if( file.read( buffer.data( ), size ) )
	{
		unsigned char seg[4];
		int position = 0x1C - 4;

		Read4Bytes( seg, buffer, position );
		int ofs = GetIntFromChunk( seg );
		Read4Bytes( seg, buffer, position + ofs );
		ofs = GetIntFromChunk( seg );
		wcout << L"->" + ReadUnicode( buffer, ofs ) + L"\n";

		position += 4;

		Read4Bytes( seg, buffer, position );
		ofs = GetIntFromChunk( seg );

		Read4BytesReversed( seg, buffer, position + ofs );
		float f;
		memcpy( &f, &seg, sizeof( f ) );
		wcout << ToString( f ) + L"\n";

		position += 4;

		Read4Bytes( seg, buffer, position );
		ofs = GetIntFromChunk( seg );
		wcout << ReadUnicode( buffer, ofs ) + L"\n------------------------------------\n";

		//
		position = 0x3c - 12;

		//Next node?
		int numNodes = 0;
		for( int i = 0; i < numNodes; i++ )
		{
			position += 12;
			Read4Bytes( seg, buffer, position );
			ofs = GetIntFromChunk( seg );
			wcout << ReadUnicode( buffer, ofs ) + L"\n";
			position += 4;

			Read4Bytes( seg, buffer, position );
			ofs = GetIntFromChunk( seg );
			wcout << ReadUnicode( buffer, ofs ) + L"\n";
			position += 4;
			position += 4;

			Read4Bytes( seg, buffer, position );
			ofs = GetIntFromChunk( seg );

			for( int j = 0; j < 4; ++j )
			{
				Read4BytesReversed( seg, buffer, ofs + ( i * 4 ) );

				float f;
				memcpy( &f, &seg, sizeof( f ) );
				wcout << ToString( f ) + L" ";
			}
			wcout << L"\n";
			position += 4;

			Read4Bytes( seg, buffer, position );
			ofs = GetIntFromChunk( seg );

			for( int j = 0; j < 4; ++j )
			{
				Read4BytesReversed( seg, buffer, ofs + ( i * 4 ) );

				float f;
				memcpy( &f, &seg, sizeof( f ) );
				wcout << ToString( f ) + L" ";
			}
			wcout << L"\n";
			position += 4;

			Read4Bytes( seg, buffer, position );
			ofs = GetIntFromChunk( seg );

			for( int j = 0; j < 4; ++j )
			{
				Read4BytesReversed( seg, buffer, ofs + ( j * 4 ) );

				float f;
				memcpy( &f, &seg, sizeof( f ) );
				wcout << ToString( f ) + L" ";
			}

			wcout << L"\n";
		}

		system( "pause" );
	}
}

int _tmain( int argc, wchar_t* argv[] )
{
    using namespace std;

	init_locale( );
	wstring path;

	if( argc > 1 )
	{
		if( !lstrcmpW( argv[1], L"/ARCHIVE" ) && argc > 2 )
		{
			std::unique_ptr< RAB > rabReader = std::make_unique< RAB >( );

			rabReader->bUseFakeCompression = false;

			int fileArgNum = 2;

			if( argc > 3 && !lstrcmpW( argv[2], L"-fc" ) )
			{
				rabReader->bUseFakeCompression = true;
				fileArgNum++;
			}

			rabReader->CreateFromDirectory( argv[fileArgNum] );

			wstring fileName = argv[fileArgNum];
			fileName += L".rab";

			rabReader->Write( fileName );
			rabReader.reset( );

			return 0;
		}

		wcout << L"Parsing file: " << argv[1] << L'\n';

		ProcessFile( std::wstring( argv[1] ), 1 );
		/*
		std::wcout << L"Compile (0) or decompile (1)?: ";
		std::wcin >> path;

		if( stoi( path ) == 0 )
		{
			CMissionScript *script = new CMissionScript( );
			std::wstring strn = argv[1];
			size_t lastindex = strn.find_last_of( L"." );
			strn = strn.substr( 0, lastindex );

			//lastindex = strn.find_last_of( L"\\" );
			//strn = strn.substr( lastindex + 1, strn.size() - lastindex );

			script->Write( strn, 1 );
			delete script;
		}
		if( stoi( path ) == 1 )
		{
			std::wstring strn = argv[1];
			size_t lastindex = strn.find_last_of( L"." );
			strn = strn.substr( 0, lastindex );

			CMissionScript *script = new CMissionScript( );
			script->Read( strn );
			delete script;
			std::wcout << "\n";
		}
		*/
	}
	else
	{
		//wcout << TestProccess( L"2+4-(10-2*(10-5)+5)+(5-2/5)" );
		//system( "pause" );
		//return 0;

		//TestReadMab( );
		//TestReadMBD( );
		//system( "pause" );
		//return 0;

		//unique_ptr<CWpnListMgr> ui = make_unique<CWpnListMgr>( );
		//ui->GenerateUI( );

		//CJSONAMLParser *parser = new CJSONAMLParser( L"EDF5MissionCommands.jsonaml" );
		//wcout << parser->SearchTest( );
		//delete parser;

		//return 0;

		//std::unique_ptr< RAB > rabReader = std::make_unique< RAB >( );
		//rabReader->CreateFromDirectory( L"NewMissionImageList" );
		//rabReader->Write( L"NewMissionImageList.rab" );
		//rabReader.reset( );

		//std::unique_ptr< SGO > sgoReader = std::make_unique< SGO >( );
		//sgoReader->Read( L"2017_example.sgo" );
		//sgoReader.reset( );

		//system( "pause" );

		//return 0;

		wcout << L"Filename:";
		wcin >> path;
		wcout << L"\n";

		wcout << L"Parsing file...\n";

		ProcessFile( path, 1 );

		
		//TEMP CMPL TEST:
		/*
		std::vector< char > inFile;
		std::ifstream file( path, std::ios::binary | std::ios::ate );

		std::streamsize size = file.tellg( );
		file.seekg( 0, std::ios::beg );

		if( size == -1 )
			return 0;

		std::vector<char> buffer( size );
		if( file.read( buffer.data( ), size ) )
		{
			CMPLHandler compressor = CMPLHandler( buffer );
			std::vector< char > comprFile = compressor.Decompress( );

			std::ofstream file = std::ofstream( L"decompressed_" + path, std::ios::binary | std::ios::out | std::ios::ate );
			for( int i = 0; i < comprFile.size(); ++i )
			{
				file << comprFile[i];
			}
			file.close( );

			comprFile.clear( );
			compressor.data.clear( );
		}
		file.close( );
		*/

		wcout << "\n";
	}
	system( "pause" );
	
	return 0;
}

