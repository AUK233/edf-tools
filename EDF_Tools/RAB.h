#pragma once


struct RABFile
{
	RABFile::RABFile( std::wstring name, int fID, std::wstring fullPath );
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
	void Read( std::wstring path, bool isMRAB );

	//Write
	void CreateFromDirectory( std::wstring path );

	void AddFilesInDirectory( std::wstring path );
	void AddFile( std::wstring filePath );
	void Write( std::wstring rabName );

	//Tool properties.
	bool bUseFakeCompression;

	//Stored Data
	int numFiles;
	int numFolders;
	int nameTablePos;
	int fileTreeStructPos;
	int dataStartOfs;

	int largestFileSize;

	std::vector< std::wstring> folders;
	std::vector< std::unique_ptr<RABFile> > files;
};
