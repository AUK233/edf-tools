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
#include "ModManager.h"

#include "Middleware.h" //Data middleware

#include "SGO.h" //SGO parser
#include "DSGO.h" //DSGO parser
#include "MAB.h" //MAB parser
#include "MTAB.h" //MTAB parser
#include "MDB.h" //MDB parser
#include "CAS.h" //CAS parser
#include "CANM.h" //CANM parser
#include "RMPA6.h" //EDF6's RMPA
#include "RAB.h" //RAB extractor
#include "MissionScript.h" //TODO: Implement mission script class that stores and proccess data


#define FLAG_VERBOSE 1
#define FLAG_CREATE_FOLDER 2

//Keep this here for now
//#define TOOL_RABARCHIVER 1

void ProcessFile( const std::wstring& path, int extraFlags )
{
    using namespace std;

	//Get file extension:
	size_t lastindex = path.find_last_of( L'.' );

	if( lastindex != wstring::npos )
	{ 
		wstring extension = path.substr( lastindex + 1, path.size( ) - lastindex );
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
            //unique_ptr<CRMPA> rmpa = make_unique<CRMPA>();
			//rmpa->Read( strn );
			unique_ptr<RMPA6> rmpa = make_unique<RMPA6>();
			rmpa->Read( strn );
			rmpa.reset();
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
			rabReader->Read( strn, L".rab");
			rabReader.reset( );
		}
		else if( extension == L"mrab" )
		{
			std::unique_ptr< RAB > rabReader = std::make_unique< RAB >( );
			rabReader->Read( strn, L".mrab");
			rabReader.reset( );
		}
		else if( extension == L"efarc" )
		{
			// Has a fatal bug, don't read efarc in non-RAMDisk.
			std::unique_ptr< RAB > rabReader = std::make_unique< RAB >( );
			rabReader->Read( strn, L".efarc");
			rabReader.reset( );
		}
		else if (extension == L"mdb")
		{
			/*
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
			*/

			unique_ptr<CMDBtoXML> script = make_unique<CMDBtoXML>();
			script->Read(strn, true);
			script.reset();
		}
		else if (extension == L"sgo")
		{
			std::unique_ptr< SGO > sgoReader = std::make_unique< SGO >();
			sgoReader->Read(strn);
			sgoReader.reset();
		}
		else if (extension == L"mab")
		{
			std::unique_ptr< MAB > mabReader = std::make_unique< MAB >();
			mabReader->Read(strn);
			mabReader.reset();
		}
		else if (extension == L"mtab")
		{
			std::unique_ptr< MTAB > mtabReader = std::make_unique< MTAB >();
			mtabReader->Read(strn);
			mtabReader.reset();
		}
		else if (extension == L"cas")
		{
			unique_ptr<CAS> casReader = make_unique<CAS>();
			casReader->Read(strn);
			casReader.reset();
		}
		else if (extension == L"canm")
		{
			unique_ptr<CANM> canmReader = make_unique<CANM>();
			canmReader->Read(strn);
			canmReader.reset();
		}
		else if (extension == L"xml")
		{
			size_t xmlIndex = strn.find_last_of(L'_');
			wstring xmlExtension = strn.substr(xmlIndex + 1, strn.size() - xmlIndex);
			wstring xmlStrn = strn.substr(0, xmlIndex);

			xmlExtension = ConvertToLower(xmlExtension);

			if (xmlExtension == L"mdb")
			{
				// To MDB File, no need for multi-core.
				unique_ptr< CXMLToMDB > script = make_unique< CXMLToMDB >();
				script->Write(xmlStrn, false);
				script.reset();
			}
			else if (xmlExtension == L"cas")
			{
				unique_ptr< CAS > script = make_unique< CAS >();
				script->Write(xmlStrn);
				script.reset();
			}
			else if (xmlExtension == L"canm")
			{
				unique_ptr< CANM > script = make_unique< CANM >();
				script->Write(xmlStrn);
				script.reset();
			}
			else if (xmlExtension == L"data")
			{
				//Data needs a function to judge the header.
				CheckXMLHeader(xmlStrn);
			}
			else if (xmlExtension == L"rmpa")
			{
				unique_ptr< RMPA6 > script = make_unique< RMPA6 >();
				script->Write(xmlStrn);
				script.reset();
			}
		}
	}
	else
	{
		//Search for valid files:
	}
}


int _tmain( int argc, wchar_t* argv[] )
{
	init_locale( );
	std::wstring path;

	if( argc > 1 )
	{
		/*if( !lstrcmpW( argv[1], L"/ARCHIVE" ) && argc > 2 )
		{
			std::unique_ptr< RAB > rabReader = std::make_unique< RAB >( );
			rabReader->Initialization();
			int fileArgNum = 2;

			if( argc > 3)
			{
				if (!lstrcmpW(argv[2], L"-fc")) {
					rabReader->bUseFakeCompression = true;
					fileArgNum++;
				}
				else if (!lstrcmpW(argv[2], L"-mt")) {
					rabReader->bIsMultipleThreads = true;
					fileArgNum++;
				}
				else if (!lstrcmpW(argv[2], L"-mc")) {
					rabReader->WriteInitMTInfo();
					fileArgNum++;
				}
				else if (!lstrcmpW(argv[2], L"-cmtn")) {
					rabReader->bIsMultipleThreads = true;
					rabReader->bIsMultipleCores = true;
					rabReader->customizeThreads = 4;
					fileArgNum++;
					if (argc > 4) {
						int tempThreadNum = std::stoi(argv[3]);
						if (tempThreadNum > 16) {
							tempThreadNum = 16;
						}
						rabReader->customizeThreads = tempThreadNum;
						fileArgNum++;
					}
				}
			}

			std::wstring fileName = argv[fileArgNum];

			rabReader->CreateFromDirectory(fileName);

			if (rabReader->esbFileNum) {
				fileName += L".efarc";
				goto writeRAB;
			}

			if (rabReader->mdbFileNum > 1)
			{
				fileName += L".mrab";
			} else {
				fileName += L".rab";
			}

			writeRAB:
			rabReader->Write( fileName );
			rabReader.reset( );

			return 0;
		}

		std::wcout << L"Parsing file: " << argv[1] << L'\n';

		ProcessFile( std::wstring( argv[1] ), 1 );*/

		if (argc > 2) {
			// to batch read
			if (!lstrcmpW(argv[1], L"-br")) {
				path = argv[2];
				ProcessFile(path, ProcessType_t::Batch, ThreadType_t::Read, 0);
				return 0;
			}

			// to batch write
			if (!lstrcmpW(argv[1], L"-bw")) {
				path = argv[2];
				ProcessFile(path, ProcessType_t::Batch, ThreadType_t::Write, 0);
				return 0;
			}

			// to pack folder
			if (argc > 3) {
				auto processType = ProcessType_t::None;
				if (!lstrcmpW(argv[1], L"-pd")) {
					processType = ProcessType_t::Pack;
				} else if(!lstrcmpW(argv[1], L"-px")) {
					processType = ProcessType_t::BatchToPackage;
				}

				if (processType == ProcessType_t::None) {
					std::wcout << L"Invalid command line!\n";
					system("pause");
					return 0;
				}

				int threadNum = 0;
				auto threadType = ThreadType_t::None;
				int argIndex = 3;

				// -in as default
				if (!lstrcmpW(argv[2], L"-fc")) {
					threadType = ThreadType_t::NoCompression;
				}
				else if (!lstrcmpW(argv[2], L"-mc")) {
					threadType = ThreadType_t::MultiCore;
				}
				else if (!lstrcmpW(argv[2], L"-mt")) {
					threadType = ThreadType_t::MultiThreading;
				}
				else if (argc > 4 && !lstrcmpW(argv[2], L"-st")) {
					threadType = ThreadType_t::SetThread;
					threadNum = 4;
					int tempThreadNum = std::stoi(argv[3]);
					if (tempThreadNum > 16) {
						tempThreadNum = 16;
					}
					threadNum = tempThreadNum;
					argIndex++;
				}

				path = argv[argIndex];
				ProcessFile(path, processType, threadType, threadNum);

				return 0;
			}
		} else {
			path = argv[1];
			ProcessFile(path, ProcessType_t::None, ThreadType_t::None, 0);
		}
	}
	else
	{
		std::wcout << L"Filename:";
		std::wcin >> path;
		std::wcout << L"\n";

		ProcessFile( path, ProcessType_t::None, ThreadType_t::None, 0 );
	}
	
	return 0;
}

