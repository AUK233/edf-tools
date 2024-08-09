#pragma once

struct RABMTFile
{
	HANDLE hnd;
	uint32_t isActive;
	size_t size;
	std::vector< char > data;
	BYTE pad[40];
};

struct RABFileList
{
	RABFileList* next;
	RABMTFile* pTask;
	std::wstring fileName;
	std::vector< char > data;
	RABFileList* pList;
	BYTE pad[16];
};

struct RABMTParameter
{
	LPCRITICAL_SECTION cs;
	RABMTFile* task;
	size_t taskNum;
	size_t index;
	std::wstring fileName;
	std::vector< char > data;
	RABFileList* pList;
	BYTE pad[8];
};

DWORD WINAPI RABWriteMTCompress(LPVOID lpParam);
DWORD WINAPI RABWriteMTCompress2(LPVOID lpParam);
RABFileList* __fastcall RABWriteMTCompressNext(RABFileList* File, LPCRITICAL_SECTION cs);

struct RABFile
{
	RABFile::RABFile( std::wstring name, int fID, const std::wstring& fullPath );
	//void LoadData( std::string path );

	std::wstring fileName;
	int fileSize;
	FILETIME fileTime;
	int fileStart;

	int fileID;
	int folderID;

	std::vector< char > data;
};

//CMPL Decompressor
struct CMPLHandler
{
	CMPLHandler( std::vector< char > inFile, bool useFakeCompression = false );

	std::vector< char > Decompress( );
	std::vector< char > Compress( );

	std::vector< char > data;

	//Tool data
	bool bUseFakeCompression;
};

struct RAB
{
	//Read
	void Read( const std::wstring& path, bool isMRAB );

	//Write
	void CreateFromDirectory( const std::wstring& path );

	void AddFilesInDirectory( const std::wstring& path );
	void AddFile( std::wstring filePath );
	void Write( const std::wstring& rabName );

	//Tool properties.
	bool bUseFakeCompression;
	bool bIsMultipleThreads;
	bool bIsMultipleCores;
	int customizeThreads;

	//Stored Data
	int numFiles;
	int numFolders;
	int nameTablePos;
	int fileTreeStructPos;
	int dataStartOfs;

	int largestFileSize;

	int mdbFileNum;

	std::vector< std::wstring> folders;
	std::vector< std::unique_ptr<RABFile> > files;
};
