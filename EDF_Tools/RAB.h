#pragma once

// Need to be a multiple of 32 bytes
struct RABMTFile
{
	HANDLE hnd;
	uint32_t isActive;
	BYTE pad1[4];
	size_t size;
	std::vector< char > data;
	BYTE pad2[16];
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
};

DWORD WINAPI RABWriteMTCompress(LPVOID lpParam);
DWORD WINAPI RABWriteMTCompress2(LPVOID lpParam);
RABFileList* __fastcall RABWriteMTCompressNext(RABFileList* File, LPCRITICAL_SECTION cs);


struct RABPreprocessFile
{
	std::wstring fileName;
	std::wstring filePath;

	bool operator<(const RABPreprocessFile& other) const {
		return fileName < other.fileName;
	}
};

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

class RAB
{
public:
	//Read
	void Read(const std::wstring& path, const std::wstring& suffix);

	//Write
	void Initialization();
	void CreateFromDirectory( const std::wstring& path );

	void AddFilesInDirectory( const std::wstring& path );
	void AddFile( std::wstring filePath );
	void Write( const std::wstring& rabName );
	DWORD CountSetBits(ULONG_PTR bitMask);
	void WriteInitMTInfo();

	//Tool properties.
	bool bUseFakeCompression;
	bool bIsMultipleThreads;
	bool bIsMultipleCores;
	int customizeThreads;
	//Stored Data
	int numFiles;
	int numFolders;
	int mdbFileNum;
	int esbFileNum;

private:
	int nameTablePos;
	int fileTreeStructPos;
	int dataStartOfs;

	int largestFileSize;

	std::vector< std::wstring> folders;
	std::vector< std::unique_ptr<RABFile> > files;
};
