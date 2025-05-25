#pragma once

struct AWEHeader_t {
	UINT32 header;
	// always 256?
	INT32 version;
	INT32 sound_count;
	int nameTable_offset;
	int fileTable_offset;
};

class AWB {
public:
	struct Header_t {
		UINT32 header;
		// should be is 1, but 2 is usable
		BYTE pad4;
		BYTE dataOfs_size;
		// usually 0x02, rarely 0x04
		UINT16 cueID_size;
		INT32 sound_count;
		UINT16 align_size;
		UINT16 subkey;
	};

	struct FileOffset_t {
		int cueID;
		int self;
		int next;
		int size;
	};

	void Read(const std::string& inPath);
	void ReadDataOffset(const std::vector<char>& buffer);
	void ReadNameTable(const std::vector<char>& buffer);
	void ReadDataToFile(const std::string& inPath, const std::vector<char>& buffer);


	// this is used to write
	struct PreprocessFileData_t {
		std::vector<char> data;
		std::string name;
		// is data offset in AWB
		int offset_data;
		// is name offset in AWE
		int offset_name;
		// is data index in AWB
		int index;
		// can be randomized, but not duplicated
		int CueID;
	};

	void Write(const std::string& inPath);
	void WriteInitHeader();
	void WriteAWBFile(const std::string& inPath, int block_CueIDSize, int block_DataOfsSize, int waveformEndSize);
	void WriteAWEFile(const std::string& inPath, int block_NameOfsSize, int block_IndexSize);

	Header_t v_header;
	std::vector<FileOffset_t> v_DataFile;
private:
	AWEHeader_t v_AWEheader;

	std::vector<std::string> v_Name;
	std::vector<UINT16> v_NameTable;

	// this is used to write
	std::vector<PreprocessFileData_t> v_File;
};
