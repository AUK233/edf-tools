#pragma once
#include "..\EDF_Tools\include\tinyxml2.h"
#include <unordered_map>

typedef std::unordered_map<std::string, int> StringToIntMap;

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

struct ACB_PassParameters {
	// ACB file path
	std::string path;
	// AWB in ACB
	std::vector<char> awb_memory;

	// to write
	std::vector<char> awb_stream;
};

struct UTF_WaveformName_t {
	std::vector<std::string> v_name;

	StringToIntMap MemoryAwb_map;
	StringToIntMap StreamAwb_map;
};

struct UTF_Sequence_t {
	std::string cue_name;
	std::vector<int> v_waveform;
};

struct UTF_Command_t {
	INT16 cmdType;
	INT8 valueSize;
};

class ACB {
public:

	void Read(const std::string& inPath);
	void ReadUTFData(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader, int IsHeader);
	void ReadUTFHeaderData(const std::vector<char>& buffer);
	// we need to check if first name is used
	std::string ReadUTF1stString(const std::vector<char>& in, int inStart, int inEnd);
	void CheckStringIsUsed(const std::string& in);
	void ReadUTFParametersList(const std::vector<char>& in, int inSize);
	// about type: 0 is fixed value, 1 is variable, 2 is acb header
	void ReadUTFParameterData(const std::vector<char>& in, tinyxml2::XMLElement* xmldata, int index, int type);
	// 
	struct UTF_GetParameters {
		tinyxml2::XMLElement* xmlNode;
		int index;
		// input parameter type, which determines what parameter is output 
		int type;
		// parameter type, whether it is a variable
		int isNodePtr;
		// now, write parameter data offset in here
		int PtrDataOfs;
	};
	void ReadUTFParameter_Value(const std::vector<char>& in, const UTF_GetParameters& inPtr);
	void ReadUTFParameter_Int(const std::vector<char>& in, const UTF_GetParameters& inPtr, int dataType);
	void ReadUTFParameter_FP32(const std::vector<char>& in, const UTF_GetParameters& inPtr);
	void ReadUTFParameter_String(const std::vector<char>& in, const UTF_GetParameters& inPtr);
	void ReadUTFParameter_ToData(const std::vector<char>& in, const UTF_GetParameters& inPtr);
	void ReadUTFData_CommandCode(const std::vector<char>& in, tinyxml2::XMLElement* xmldata);
	// get waveform name
	void ReadAWBWaveformName(tinyxml2::XMLElement* xmlHeader, UTF_WaveformName_t* pName);
	tinyxml2::XMLElement* ReadAWBWaveformName_GetNode(tinyxml2::XMLElement* xmldata, const char* name);
	void ReadAWBWaveformName_GetSequence(tinyxml2::XMLElement* xmldata, std::vector<UTF_Sequence_t>& v_Sequence);
	void ReadAWBWaveformName_GetCueName(tinyxml2::XMLElement* xmldata, std::vector<UTF_Sequence_t>& v_Sequence);
	// read AWB data
	void ReadAWBData(const std::vector<char>& in, tinyxml2::XMLElement* xmldata);

	// write ACB
	void Write(const std::string& inPath);
	// get awb data
	void WriteAWBDataCheck(tinyxml2::XMLElement* xmldata);
	std::vector<char> WriteAWBDataGet(tinyxml2::XMLElement* xmldata, int* headerSize);
	// get utf data
	std::vector<char> WriteUTFData(tinyxml2::XMLElement* xmldata, int isHeader);
	void WriteUTFData_InitHeader(UTF_Header_t** p);
	int WriteUTFData_WriteString(std::vector<char>& buffer, StringToIntMap& map, const std::string& str);
	//
	struct UTF_Data_ptr_t {
		int data_type;
		int data_size;
		int data_offset;
	};
	//
	struct UTF_Data_t {
		// parameter block
		std::vector<char> v_parameter;
		// node block
		std::vector<char> v_node;
		// data block
		std::vector<char> v_data;
		// string table
		std::vector<char> v_string;
		StringToIntMap string_map;

		// parameter table
		std::vector<UTF_Data_ptr_t> v_ptrTable;
		StringToIntMap ptrTable_map;

		// variable data block size
		int var_size;
		//
		int parameter_count;
		int node_count;
	};
	void WriteUTFData_InitParameterName(tinyxml2::XMLElement* xmldata, UTF_Data_t* p_data);
	// about type: 0 is fixed value, 1 is variable, 2 is acb header
	void WriteUTFNode(tinyxml2::XMLElement* xmldata, UTF_Data_t* p_data, int type);
	// get parameter list, and write fixed or header data.
	void WriteUTFParameter(tinyxml2::XMLElement* xmldata, UTF_Data_t* p_data, int isHeader);
	// 
	struct UTF_PTRData_t {
		int data_type;
		int data_size;
		char buffer[8];
	};
	void WriteUTFParameterData(tinyxml2::XMLElement* xmldata, UTF_Data_t* p_data, UTF_PTRData_t* p);
	void WriteUTFNodeData(tinyxml2::XMLElement* xmldata, UTF_Data_t* p_data, std::vector<char>& buffer);
	void WriteUTFNodePTRData(tinyxml2::XMLElement* xmldata, UTF_Data_t* p_data, char* buffer);
	std::vector<char> WriteUTF_CommandData(tinyxml2::XMLElement* xmldata, int* pOutSize);

	ACB_PassParameters* common_parameter = 0;
private:
	UTF_Header_t v_header;
	std::vector<UTF_Column_t> v_parameters;
	// for name table
	StringToIntMap name_map;
};