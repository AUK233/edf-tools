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
		int self;
		int next;
	};

	void Read(const std::string& inPath);
	void ReadDataOffset(const std::string& inPath, const std::vector<char>& buffer);
	void ReadNameTable(const std::vector<char>& buffer);
	void ReadDataToFile(const std::string& inPath, const std::vector<char>& buffer);

private:
	Header_t v_header;
	AWEHeader_t v_AWEheader;

	std::vector<FileOffset_t> v_DataFile;
	std::vector<std::string> v_Name;
	std::vector<UINT16> v_NameTable;
};