#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include "util.h"
#include "RAB.h"

//#define RABREADER_DEBUG

void RAB::Read( std::wstring path, bool isMRAB )
{
	std::wstring ext;
	if( isMRAB )
		ext = L".mrab";
	else
		ext = L".rab";

	std::ifstream file( path + ext, std::ios::binary | std::ios::ate );

	std::streamsize size = file.tellg( );
	file.seekg( 0, std::ios::beg );

	if( size == -1 )
		return;

	std::vector<char> buffer( size );
	if( file.read( buffer.data( ), size ) )
	{
#ifndef RABREADER_DEBUG
		//Create folder

		//Needed for directory stuff.
		std::wstring directory;

		size_t last_slash_idx = path.rfind( L'\\' );
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

		std::wstring myFolderName;
		std::wstring oldFolder;
		last_slash_idx = directory.rfind( L'\\' );
		if( std::string::npos != last_slash_idx )
		{
			myFolderName = directory.substr( last_slash_idx + 1, directory.size( ) - last_slash_idx );
			oldFolder = directory.substr( 0, last_slash_idx );
		}

		CreateDirectoryExW( (oldFolder).c_str( ), path.c_str( ), NULL );
#endif

		//Begin read
		unsigned char seg[4];
		int position;

		Read4Bytes( seg, buffer, 0x8 );

		dataStartOfs = GetIntFromChunk( seg );
		std::wcout << L"Data starting at " + ToString( dataStartOfs ) + L"\n";

		Read4Bytes( seg, buffer, 0x14 );
		numFiles = GetIntFromChunk( seg );
		std::wcout << L"Number of archived files: " + ToString( numFiles ) + L"\n";

		Read4Bytes( seg, buffer, 0x1c );
		fileTreeStructPos = GetIntFromChunk( seg );
		std::wcout << L"File Tree Struct position: " + ToString( fileTreeStructPos ) + L"\n";

		Read4Bytes( seg, buffer, 0x20 );
		numFolders = GetIntFromChunk( seg );
		std::wcout << L"Number of archived Folders: " + ToString( numFolders ) + L"\n";

		Read4Bytes( seg, buffer, 0x24 );
		nameTablePos = GetIntFromChunk( seg );
		std::wcout << L"Name Table position: " + ToString( nameTablePos ) + L"\n";

		//Read folders:
		position = nameTablePos;

		for( int i = 0; i < numFolders; ++i )
		{
			Read4Bytes( seg, buffer, position );
			folders.push_back( ReadUnicode( buffer, position + GetIntFromChunk( seg ) ) );
			std::wcout << L"FOLDER " + ToString( i ) + L":" + folders.back() + L"\n";

#ifndef RABREADER_DEBUG
			//Create folder:
			std::wstring newFolderPath = path + L"\\" + folders.back( ).c_str( );
			CreateDirectoryExW( ( oldFolder ).c_str( ), newFolderPath.c_str(), NULL );
#endif

			position += 4;
		}

		//Read files:
		position = 0x28;
		for( int i = 0; i < numFiles; ++i )
		{
			std::wcout << L"\n";
			std::wcout << L"FILE: ";
			Read4Bytes( seg, buffer, position );
			std::wstring fileName = ReadUnicode( buffer, position + GetIntFromChunk( seg ) );
			std::wcout << fileName + L"\n";

			position += 0x4;

			Read4Bytes( seg, buffer, position );
			int fileSize = GetIntFromChunk( seg );
			std::wcout << L"--FILE SIZE: " + ToString( fileSize ) + L"\n";

			position += 0x4;

			Read4Bytes( seg, buffer, position );

			std::wstring folderName = folders.at( GetIntFromChunk( seg ) );

			std::wcout << L"--PARENT NAME: " + folderName + L"\n";

			position += 0x4;

			position += 0x4;

			unsigned char data[8];

			Read4BytesReversed( seg, buffer, position );
			data[0] = seg[0];
			data[1] = seg[1];
			data[2] = seg[2];
			data[3] = seg[3];

			position += 0x4;
			Read4BytesReversed( seg, buffer, position );

			data[4] = seg[0];
			data[5] = seg[1];
			data[6] = seg[2];
			data[7] = seg[3];

			FILETIME ft;
			memcpy( &ft, data, sizeof( FILETIME ) );

			SYSTEMTIME st;

			FileTimeToSystemTime( &ft, &st );

			std::wstring fileTimeString;

			fileTimeString += ToString( st.wHour ) + L":" + ToString( st.wMinute ) + L" ";
			fileTimeString += ToString( st.wDay ) + L"/";
			fileTimeString += ToString( st.wMonth ) + L"/";
			fileTimeString += ToString( st.wYear );

			std::wcout << L"--FILE TIME: " + fileTimeString + L"\n";
			position += 0x4;

			Read4Bytes( seg, buffer, position );
			int fileStart = GetIntFromChunk( seg );
			std::wcout << L"--CONTENT START POS: " + ToString( fileStart ) + L"\n";

			position += 0x4;

			//Unknown block:
			std::wcout << L"--UNKNOWN BLOCK: ";
			Read4BytesReversed( seg, buffer, position );
			for( int j = 0; j < 4; ++j )
			{
				std::wcout << L"0x";
				std::wcout << std::hex << seg[j];
				std::wcout << L" ";
			}
			std::wcout << L"\n";

			position += 0x4;

#ifndef RABREADER_DEBUG
			std::wstring correctedPath = path + L"\\" + folderName + L"\\" + fileName;

			//Extract a file to a local file vector:
			std::vector< char > tempFile;
			for( int j = 0; j < fileSize; ++j )
			{
				tempFile.push_back(buffer[fileStart + j]);
			}

			bool shouldDecompress = true;

			if( shouldDecompress )
			{
				//Decompress
				CMPLHandler decompresser = CMPLHandler( tempFile );
				std::vector< char > decompressedFile = decompresser.Decompress( );
				tempFile.clear( );

				//Output
				std::ofstream file = std::ofstream( correctedPath, std::ios::binary | std::ios::out | std::ios::ate );
				for( int j = 0; j < decompressedFile.size(); ++j )
				{
					file << decompressedFile[j];
				}
				file.close( );

				decompressedFile.clear( );
			}
			else
			{
				//Output
				std::ofstream file = std::ofstream( correctedPath, std::ios::binary | std::ios::out | std::ios::ate );
				for( int j = 0; j < tempFile.size( ); ++j )
				{
					file << tempFile[j];
				}
				file.close( );
			}

			//A tad hacky.
			HANDLE fHandle = CreateFileW( (LPCWSTR)correctedPath.c_str( ),
				FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL, NULL );

			//Set the filetime on the file
			SetFileTime( fHandle, (LPFILETIME)NULL, (LPFILETIME)NULL, &ft );

			//Close our handle.
			CloseHandle( fHandle );
#endif
		}
	}
}

void RAB::CreateFromDirectory( std::wstring path )
{
	largestFileSize = 0;

	//Scan folders in directory:

	WIN32_FIND_DATA fileData;
	HANDLE hFind = FindFirstFile( (path + L"\\*" ).c_str( ), &fileData );
	if( hFind == INVALID_HANDLE_VALUE )
	{
		std::wcout << "BAD PATH IN RAB WRITE!\n";
		return;
	}

	do
	{
		if( fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			//Check if it is a control path:
			if( std::wcscmp( fileData.cFileName, L"." ) && std::wcscmp( fileData.cFileName, L".." ) )
			{
				//Todo: More methods of doing this.
				//Exclude certain files.
				if( lstrcmpW( fileData.cFileName, L"Excluded" ) && lstrcmpW( fileData.cFileName, L"Exclude" ) )
				{
					folders.push_back( fileData.cFileName );
					AddFilesInDirectory( path + L"\\" + fileData.cFileName );
				}
			}
		}
	} while( FindNextFile( hFind, &fileData ) != 0 );

	FindClose( hFind );
}

#ifdef MULTITHREAD
#include <thread> //Move this to head of file.

//Multithreaded file compressor.
#define MAX_MULTITHREADED_FILES 4

void MultithreadCompressFile( RABFile *file )
{
	std::wcout << L"Compressing file: " + file->fileName + L"\n";

	CMPLHandler compresser = CMPLHandler( file->data );
	compresser.bUseFakeCompression = false;

	std::vector< char > compressedFile = compresser.Compress( );

	file->data.clear( );
	file->data = compressedFile;
}

#endif

void RAB::AddFilesInDirectory( std::wstring path )
{
	std::wcout << L"Writing path " + path + L"!\n";

	//Scan files in directory:

	WIN32_FIND_DATA fileData;
	HANDLE hFind = FindFirstFile( ( path + L"\\*" ).c_str( ), &fileData );
	if( hFind == INVALID_HANDLE_VALUE )
	{
		std::wcout << "BAD PATH IN RAB WRITE!\n";
		return;
	}

#ifdef MULTITHREAD
	std::vector< std::thread > threads;
#endif

	do
	{
		if( !(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
		{
			std::wstring fileName = fileData.cFileName;
			std::wcout << L"FILE:" + fileName + L"\n";

			AddFile( path + L"\\" + fileName );

#ifdef MULTITHREAD
			if( threads.size( ) < MAX_MULTITHREADED_FILES - 1 )
			{
				threads.push_back( std::thread( MultithreadCompressFile, files.back( ).get( ) ) );
			}
			else
			{
				threads.push_back( std::thread( MultithreadCompressFile, files.back( ).get( ) ) );

				std::wcout << L"Ran Max Allowed threads. Waiting for completion\n";

				for( int i = 0; i < threads.size( ); ++i )
				{
					threads[i].join( );

					std::wcout << L"Completed All Threads. Continuing\n";
				}
			}
#endif
		}
	} while( FindNextFile( hFind, &fileData ) != 0 );

	FindClose( hFind );
}

void RAB::AddFile( std::wstring filePath )
{
	std::wstring directory;
	std::wstring file = filePath;

	size_t last_slash_idx = filePath.rfind( L'\\' );
	if( std::string::npos != last_slash_idx )
	{
		directory = filePath.substr( 0, last_slash_idx );
		file = filePath.substr( last_slash_idx + 1, filePath.size( ) - last_slash_idx );
	}

	//Further split string
	last_slash_idx = directory.rfind( L'\\' );
	if( std::string::npos != last_slash_idx )
	{
		directory = directory.substr( last_slash_idx + 1, directory.size( ) - last_slash_idx );
	}

	//std::wcout << L"DGB:" + file + L"\n";

	int folderID = -1;
	for( int i = 0; i < folders.size( ); ++i )
	{
		if( folders[i] == directory )
		{
			folderID = i;
			break;
		}
	}

	files.push_back( std::make_unique< RABFile >( file, folderID, filePath ) );
	files.back( )->fileID = files.size( ) - 1;
	numFiles = files.size();

	if( files.back( )->fileSize > largestFileSize )
		largestFileSize = files.back( )->fileSize;
}

#include <algorithm>

bool CompRabNames( const RABFile *a, const RABFile *b )
{
	return a->fileName < b->fileName;
}

bool CompRabFolders( const std::unique_ptr<RABFile> &a, const std::unique_ptr<RABFile> &b )
{
	return a->folderID > b->folderID;
}

void RAB::Write( std::wstring rabName )
{
	//Sort inputs:
	std::sort( files.begin( ), files.end( ), CompRabFolders );

	for( int i = 0; i < files.size( ); ++i )
	{
		files[i]->fileID = i;
	}

	//Misc variables
	char *seg;

	//Generate data:
	std::vector< char > data;

	std::wcout << L"Beggining RAB Archiving.\n";

	//Generate header:

	//0x0: SSA
	data.push_back( 0x53 );
	data.push_back( 0x53 );
	data.push_back( 0x41 );
	data.push_back( 0x00 );

	//0x4: Unknown data, possibly version number.
	data.push_back( 0x10 );
	data.push_back( 0x01 );
	data.push_back( 0x00 );
	data.push_back( 0x00 );

	//0x8: Archive start data
	const int archiveStartDataOffs = 0x8;

	data.push_back( 0xff );
	data.push_back( 0xff );
	data.push_back( 0xff );
	data.push_back( 0xff );

	//0xc: LargestCompressedFileSize
	const int largestCompressedFileSizeOffs = 0xc;

	data.push_back( 0x00 );
	data.push_back( 0x00 );
	data.push_back( 0x00 );
	data.push_back( 0x00 );

	//0x10: Largest uncompressed file size
	seg = IntToBytes( largestFileSize );
	for( int i = 0; i < 4; ++i )
		data.push_back( seg[i] );
	free( seg );

	//0x14: Number of files:
	seg = IntToBytes( numFiles );
	for( int i = 0; i < 4; ++i )
		data.push_back( seg[i] );
	free( seg );

	//0x18: Offset to beginning of file info table, always 0x28
	data.push_back( 0x28 );
	data.push_back( 0x00 );
	data.push_back( 0x00 );
	data.push_back( 0x00 );

	//0x1C Offset to beginning of file name table
	const int fileNameTableStartOffs = 0x1c;
	data.push_back( 0x00 );
	data.push_back( 0x00 );
	data.push_back( 0x00 );
	data.push_back( 0x00 );

	//0x20 Number of Folders:
	numFolders = folders.size( );
	seg = IntToBytes( numFolders );
	for( int i = 0; i < 4; ++i )
		data.push_back( seg[i] );
	free( seg );

	//0x24 Offset to beginning of folder name table
	const int folderNameTableStartOffs = 0x24;
	data.push_back( 0x00 );
	data.push_back( 0x00 );
	data.push_back( 0x00 );
	data.push_back( 0x00 );

	//HEADER COMPLETE, PREPARE ARCHIVE TABLES:

	//File Table:
	int fileTablePos = data.size( );

	std::vector< int > fileNameStringPos;
	std::vector< int > fileOffsPos;
	std::vector< int > fileCompressedSizePos;

	for( int i = 0; i < files.size( ); ++i )
	{
		fileNameStringPos.push_back( data.size( ) );

		//0x0 Offset to file name
		data.push_back( 0x00 );
		data.push_back( 0x00 );
		data.push_back( 0x00 );
		data.push_back( 0x00 );

		//0x4 Size of file compressed

		fileCompressedSizePos.push_back( data.size( ) );

		seg = IntToBytes( files[i]->fileSize );
		for( int j = 0; j < 4; ++j )
			data.push_back( seg[j] );
		free( seg );
		
		//0x8 Folder index.
		seg = IntToBytes( files[i]->folderID );
		for( int j = 0; j < 4; ++j )
			data.push_back( seg[j] );
		free( seg );

		//0xc HD Texture?
		int isHD = 0;
		if( folders[files[i]->folderID] == L"HD-TEXTURE" )
			isHD = 1;

		seg = IntToBytes( isHD );
		for( int j = 0; j < 4; ++j )
			data.push_back( seg[j] );
		free( seg );

		//0x10 File time

		unsigned char datetime[8];

		memcpy( datetime, &files[i]->fileTime, sizeof( FILETIME ) );

		for( int j = 0; j < 8; ++j )
			data.push_back( datetime[j] );

		/*
		seg = IntToBytes( files[i]->fileTime.dwLowDateTime, false );
		for( int j = 0; j < 4; ++j )
			data.push_back( seg[j] );
		free( seg );

		seg = IntToBytes( files[i]->fileTime.dwHighDateTime, false );
		for( int j = 0; j < 4; ++j )
			data.push_back( seg[j] );
		free( seg );
		*/

		//0x18 file content offs
		fileOffsPos.push_back( data.size( ) );

		data.push_back( 0x00 );
		data.push_back( 0x00 );
		data.push_back( 0x00 );
		data.push_back( 0x00 );

		//0x1c Unknown Data
		data.push_back( 0x00 );
		data.push_back( 0x00 );
		data.push_back( 0x00 );
		data.push_back( 0x00 );
	}

	//Sorted file name table (Will not sort, don't care enough to. Should be fine.)
	int fileNameTablePos = data.size( );

	std::vector< int > fileNameTableStringPos;

	std::vector< RABFile * > sortedFiles;
	for( int i = 0; i < files.size( ); ++i )
	{
		sortedFiles.push_back( files[i].get() );
	}

	//Sort all files:
	std::sort( sortedFiles.begin( ), sortedFiles.end( ), CompRabNames );

	for( int i = 0; i < sortedFiles.size( ); ++i )
	{
		fileNameTableStringPos.push_back( data.size( ) );

		//0x0 Offset to file name
		data.push_back( 0x00 );
		data.push_back( 0x00 );
		data.push_back( 0x00 );
		data.push_back( 0x00 );

		//0x4 File index.
		seg = IntToBytes( sortedFiles[i]->fileID );
		for( int j = 0; j < 4; ++j )
			data.push_back( seg[j] );
		free( seg );
	}

	//Folder name table:
	int folderTablePos = data.size( );

	std::vector< int > folderNameStringPos;

	for( int i = 0; i < folders.size( ); ++i )
	{
		folderNameStringPos.push_back( data.size( ) );

		data.push_back( 0x00 );
		data.push_back( 0x00 );
		data.push_back( 0x00 );
		data.push_back( 0x00 );
	}

	//Correct file name table offs
	seg = IntToBytes( fileNameTablePos );
	for( int i = 0; i < 4; ++i )
		data[fileNameTableStartOffs + i] = seg[i];
	free( seg );

	//Correct folder table offs
	seg = IntToBytes( folderTablePos );
	for( int i = 0; i < 4; ++i )
		data[folderNameTableStartOffs + i] = seg[i];
	free( seg );

	//Strings:
	//Files
	for( int i = 0; i < files.size( ); ++i )
	{
		seg = IntToBytes( data.size( ) - fileNameStringPos[i] );
		for( int j = 0; j < 4; ++j )
			data[fileNameStringPos[i] + j] = seg[j];
		free( seg );

		for( int s = 0; s < sortedFiles.size(); ++s )
		{
			if( files[i]->fileName == sortedFiles[s]->fileName )
			{
				seg = IntToBytes( data.size( ) - fileNameTableStringPos[s] );
				for( int j = 0; j < 4; ++j )
					data[fileNameTableStringPos[s] + j] = seg[j];
				free( seg );

				break;
			}
		}

		PushWStringToVector( files[i]->fileName, &data );
	}

	sortedFiles.clear( );

	//Folders
	for( int i = 0; i < folders.size( ); ++i )
	{
		seg = IntToBytes( data.size() - folderNameStringPos[i] );
		for( int j = 0; j < 4; ++j )
			data[folderNameStringPos[i] + j] = seg[j];
		free( seg );

		PushWStringToVector( folders[i], &data );
	}

	std::wcout << L"RAB Archive data complete, now archiving files...\n";

	//Correct archive data offs
	seg = IntToBytes( data.size( ) );
	for( int i = 0; i < 4; ++i )
		data[archiveStartDataOffs + i] = seg[i];
	free( seg );

	//File contents:
	int largestCompressedFile = 0;
	for( int i = 0; i < files.size( ); ++i )
	{
		seg = IntToBytes( data.size( ) );
		for( int j = 0; j < 4; ++j )
			data[fileOffsPos[i] + j] = seg[j];
		free( seg );

		bool shouldCompress = true;
		if( shouldCompress )
		{
			std::wcout << L"Compressing file: " + files[i]->fileName + L"\n";

			CMPLHandler compresser = CMPLHandler( files[i]->data );
			compresser.bUseFakeCompression = bUseFakeCompression;

			std::vector< char > compressedFile = compresser.Compress( );

			if( largestCompressedFile < compressedFile.size( ) )
				largestCompressedFile = compressedFile.size( );

			seg = IntToBytes( compressedFile.size( ) );
			for( int j = 0; j < 4; ++j )
				data[fileCompressedSizePos[i] + j] = seg[j];
			free( seg );

			for( int j = 0; j < compressedFile.size( ); ++j )
			{
				data.push_back( compressedFile[j] );
			}
			compressedFile.clear( );
			compresser.data.clear( );
		}
		else
		{
			if( largestCompressedFile < files[i]->data.size( ) )
				largestCompressedFile = files[i]->data.size( );

			for( int j = 0; j < files[i]->data.size( ); ++j )
			{
				data.push_back( files[i]->data[j] );
			}
		}


		std::wcout << L"File: " + files[i]->fileName + L" Archived\n";
	}

	//Update "largest compressed file" int:
	seg = IntToBytes( largestCompressedFile );
	for( int i = 0; i < 4; ++i )
		data[largestCompressedFileSizeOffs + i] = seg[i];
	free( seg );

	//Write RAB
	std::ofstream file = std::ofstream( rabName, std::ios::binary | std::ios::out | std::ios::ate );
	for( int j = 0; j < data.size( ); ++j )
	{
		file << data[j];
	}
	file.close( );

	data.clear( );

	//Purge file list
	for( int i = 0; i < files.size( ); ++i )
	{
		files[i].reset( );
	}

	//Purge remaining vectors
	folderNameStringPos.clear( );
	fileNameStringPos.clear( );
	fileNameTableStringPos.clear( );
	fileCompressedSizePos.clear( );
	fileOffsPos.clear( );
	data.clear( );

	std::wcout << L"RAB Archive operation completed!\n";
}

RABFile::RABFile( std::wstring name, int fID, std::wstring fullPath )
{
	fileName = name;
	folderID = fID;
	fileID = 0;

	std::ifstream file( fullPath, std::ios::binary | std::ios::ate );

	std::streamsize size = file.tellg( );
	file.seekg( 0, std::ios::beg );

	if( size == -1 )
		return;

	std::vector<char> buffer( size );
	if( file.read( buffer.data( ), size ) )
	{
	}

	file.close( );

	fileSize = buffer.size( );
	data = buffer;

	//A tad hacky.
	HANDLE fHandle = CreateFileW( (LPCWSTR)fullPath.c_str( ),
		FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL );

	//Set the filetime on the file
	GetFileTime( fHandle, (LPFILETIME)NULL, (LPFILETIME)NULL, &fileTime );

	//Close our handle.
	CloseHandle( fHandle );
}

//CMPL Tools:
CMPLHandler::CMPLHandler( std::vector< char > inFile, bool useFakeCompression )
{
	data = inFile;

	bUseFakeCompression = useFakeCompression;
}

//CMPL Decompressor
std::vector< char > CMPLHandler::Decompress(  )
{
	//Check header:
	if( data[0] != 'C' && data[1] != 'M' && data[2] != 'P' && data[3] != 'L' )
	{
		std::wcout << L"FILE IS NOT CMPL COMPRESSED!\n";
		return data;
	}
	else
		std::wcout << L"BEGINNING DECOMPRESSION\n";

	//Variables
	uint8_t bitbuf;
	uint8_t bitbufcnt;

	std::vector< char > out;

	char mainbuf[4096];

	int bufPos;

	unsigned char seg[4];
	Read4BytesReversed( seg, data, 4 );
	int desiredSize = GetIntFromChunk( seg );

	int streamPos = 8;

	//Init buffer:
	for( int i = 0; i < 4078; ++i )
		mainbuf[i] = 0;
	bitbuf = 0;
	bitbufcnt = 0;
	bufPos = 4078;

	//Loop
	while( streamPos < data.size( ) )
	{
		//If bitbuf empty
		if( bitbufcnt == 0 )
		{
			//Read and store in buffer
			bitbuf = data[streamPos];
			bitbufcnt = 8;

			++streamPos;
		}

		//if first bit is one, copy byte from input
		if( bitbuf & 0x1 > 0 )
		{
			out.push_back( data[streamPos] );
			mainbuf[bufPos] = data[streamPos];
			++bufPos;
			if( bufPos >= 4096 )
				bufPos = 0;
			++streamPos;
		}
		else //Copy bytes from buffer
		{
			uint8_t chunk[2];
			chunk[1] = data[streamPos + 1];
			chunk[0] = data[streamPos];

			streamPos += 2;

			uint16_t val = ( chunk[0] << 8 ) | chunk[1];
			int copyLen = ( val & 0xf ) + 3;
			int copyPos = val >> 4;

			for( int i = 0; i < copyLen; ++i )
			{
				unsigned char byte = mainbuf[copyPos];
				out.push_back( byte );
				mainbuf[bufPos] = byte;

				++bufPos;
				if( bufPos >= 4096 )
					bufPos = 0;

				++copyPos;
				if( copyPos >= 4096 )
					copyPos = 0;
			}
		}

		--bitbufcnt;
		bitbuf >>= 1;
	}

	if( out.size( ) == desiredSize )
	{
		std::wcout << L"FILE SIZE MATCH! " + ToString( desiredSize ) + L" bytes expected, got " + ToString( (int)out.size( ) ) +  L" DECOMPRESSION SUCCESSFUL!\n";
	}
	else
		std::wcout << L"FILE SIZE MISMATCH! " + ToString( desiredSize ) + L" bytes expected, got " + ToString( (int)out.size( ) ) + L" DECOMPRESSION FAILED!\n";

	return out;
}

//CMPL Compresser
std::vector< char > CMPLHandler::Compress( )
{
	//Declare output
	std::vector< char > out;

	//Fill header:
	out.push_back( 'C' );
	out.push_back( 'M' );
	out.push_back( 'P' );
	out.push_back( 'L' );

	//File size
	char *sizeBytes = IntToBytes( data.size( ), false );

	out.push_back( sizeBytes[0] );
	out.push_back( sizeBytes[1] );
	out.push_back( sizeBytes[2] );
	out.push_back( sizeBytes[3] );

	free( sizeBytes );

	//Begin compression:

	//Use fake compression, faster compile times but extremly ineffecient
	if( bUseFakeCompression )
	{
		std::wcout << L"File using SIMPLE/FAKE Compression.\n";

		//Prepare to format the data into something that the game's CMPL decompressor will read, this data will be trash, not really compressed and is horrible, but just do it anyway.
		int count = 0;
		while( count < data.size( ) )
		{
			//Push "Copy 8 bits directly" command to reader
			out.push_back( 0b11111111 );

			for( int i = 0; i < 8; ++i )
			{
				if( count >= data.size( ) )
					break;
				out.push_back( data[count] );
				++count;
			}
		}
	}
	else
	{
		//CMPL Compression Algorithm by BlueAmulet
		//std::vector<uint8_t> out;
		std::vector<uint8_t> temp;
		int16_t mainbuf[4096];
		uint16_t bufPos = 4078;
		size_t streamPos = 0;
		uint8_t bits = 0;

		for( size_t i = 0; i < _countof( mainbuf ); i++ )
		{
			if( i < bufPos )
			{
				mainbuf[i] = 0;
			}
			else
			{
				// Mark end of buffer as uninitialized
				mainbuf[i] = -1;
			}
		}

		size_t dataSize = data.size( );

		while( streamPos < dataSize )
		{
			bits = 0;
			for( size_t i = 0; i < 8; i++ )
			{
				if( streamPos >= dataSize )
				{
					break;
				}
				size_t bestPos = 0;
				size_t bestLen = 0;
				// Try to find match in buffer
				// TODO: Properly support repeating data
				for( size_t jo = 0; jo < _countof( mainbuf ); jo++ )
				{
					uint16_t j = ( bufPos - jo ) & 0xFFF;
					if( mainbuf[j] == (int16_t)(uint16_t)data[streamPos] )
					{
						size_t matchLen = 0;
						for( size_t k = 0; k < 18; k++ )
						{
							if( ( streamPos + k ) < dataSize && ( ( j + k ) & 0xFFF ) != bufPos && mainbuf[( j + k ) & 0xFFF] == (int16_t)(uint16_t)data[streamPos + k] )
							{
								matchLen = k + 1;
							}
							else
							{
								break;
							}
						}
						if( matchLen > bestLen )
						{
							bestLen = matchLen;
							bestPos = j;
						}
					}
				}
				// Repeating byte check
				if( mainbuf[( bufPos - 1 ) & 0xFFF] == (int16_t)(uint16_t)data[streamPos] )
				{
					size_t matchLen = 0;
					for( size_t k = 0; k < 18; k++ )
					{
						if( ( streamPos + k ) < dataSize && mainbuf[( bufPos - 1 ) & 0xFFF] == (int16_t)(uint16_t)data[streamPos + k] )
						{
							matchLen = k + 1;
						}
						else
						{
							break;
						}
					}
					if( matchLen > bestLen ) {
						bestLen = matchLen;
						bestPos = ( bufPos - 1 ) & 0xFFF;
					}
				}
				// Is copy viable?
				if( bestLen >= 3 )
				{
					// Write copy data
					uint16_t copyVal = bestLen - 3;
					copyVal |= bestPos << 4;
					temp.push_back( copyVal >> 8 );
					temp.push_back( copyVal & 0xFF );
					for( size_t j = 0; j < bestLen; j++ ) {
						mainbuf[bufPos] = data[streamPos];
						bufPos = ( bufPos + 1 ) & 0xFFF;
						streamPos++;
					}
				}
				else
				{
					// Copy from input
					temp.push_back( data[streamPos] );
					mainbuf[bufPos] = data[streamPos];
					bufPos = ( bufPos + 1 ) & 0xFFF;
					streamPos++;
					bits |= 1 << i;
				}
			}
			out.push_back( bits );
			out.insert( out.end( ), temp.begin( ), temp.end( ) );
			temp.clear( );
		}
	}

	return out;
}

