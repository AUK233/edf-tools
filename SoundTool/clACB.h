#pragma once
#include "..\EDF_Tools\include\tinyxml2.h"

enum columna_flag_t {
	COLUMN_FLAG_NAME = 0x10,     /* column has name (may be empty) */
	COLUMN_FLAG_DEFAULT = 0x20,     /* data is found relative to schema start (typically constant value for all rows) */
	COLUMN_FLAG_ROW = 0x40,     /* data is found relative to row start */
	COLUMN_FLAG_UNDEFINED = 0x80      /* shouldn't exist */
};

enum column_type_t {
	COLUMN_TYPE_UINT8 = 0x00,
	COLUMN_TYPE_SINT8 = 0x01,
	COLUMN_TYPE_UINT16 = 0x02,
	COLUMN_TYPE_SINT16 = 0x03,
	COLUMN_TYPE_UINT32 = 0x04,
	COLUMN_TYPE_SINT32 = 0x05,
	COLUMN_TYPE_UINT64 = 0x06,
	COLUMN_TYPE_SINT64 = 0x07,
	COLUMN_TYPE_FLOAT = 0x08,
	COLUMN_TYPE_DOUBLE = 0x09,
	COLUMN_TYPE_STRING = 0x0a,
	COLUMN_TYPE_VLDATA = 0x0b,
	COLUMN_TYPE_UINT128 = 0x0c, /* for GUIDs */
	COLUMN_TYPE_UNDEFINED = -1
};

// it is BigEndian
struct UTF_Header_t {
	// @UTF
	UINT32 U32_header;
	// this data chunk's size
	// starting at 0x8, 32-bytes aligned
	// excludes aligned bytes, and header 8-bytes
	UINT32 BlockSize;
	// should be 1
	UINT16 Version;
	// starting at 0x8
	UINT16 NodeDataOffset;
	// starting at 0x8
	UINT32 NameTableOffset;
	// starting at 0x8
	UINT32 DataOffset;
	// name table? always 0?
	UINT32 NameOffset;
	// 
	UINT16 ParameterCount;
	// parameter data size for each node
	UINT16 NodeDataSize;
	//
	UINT32 NodeCount;
};

struct UTF_Column_t {
	UINT8 flag;
	UINT8 type;
	const char* name;
	UINT32 offset;
};

class ACB {
public:

	void Read(const std::string& inPath);
	void ReadUTFData(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader);
	void ReadUTFHeaderData(const std::vector<char>& buffer);
	void ReadGetStringList(const std::vector<char>& in, int inStart, int inEnd, std::vector<std::string>& out);
	void ReadUTFParametersList(const std::vector<char>& in, int inSize);

	struct UTF_GetParameters {
		tinyxml2::XMLElement* xmlNode;
		int index;
		int isNodePtr;
		// now, write parameter data offset in here
		int PtrDataOfs;
	};
	void ReadUTFParameter_Int(const std::vector<char>& in, const UTF_GetParameters& inPtr, int dataType);
	void ReadUTFParameter_FP32(const std::vector<char>& in, const UTF_GetParameters& inPtr);
	void ReadUTFParameter_String(const std::vector<char>& in, const UTF_GetParameters& inPtr);
	void ReadUTFParameter_ToData(const std::vector<char>& in, const UTF_GetParameters& inPtr);

	std::string InPath;
private:
	UTF_Header_t v_header;
	std::vector<UTF_Column_t> v_parameters;

};