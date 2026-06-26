#pragma once
//#include <map>
#include "include/tinyxml2.h"
#include "HashMap.h"
#include <unordered_map>

struct SGONode {
	int type;

	//Value types
	std::vector< SGONode * > ptrvalue;
	int ivalue;
	float fvalue;
	std::wstring strvalue;
	std::vector< char > data;
};

struct SGONodeName
{
	std::wstring name;
	// as position when writing
	int id;
};

struct SGOExtraData
{
	std::string name;
	std::vector< char > bytes;
	size_t size;
};

struct OutSGOHeader_t {
	int header = 5195603;
	int unk0x4 = 258;
	int DataNodeCount = 0;
	int DataNodeOffset = 0x20;
	int DataNameCount = 0;
	int DataNameOffset = 0x20;
	// this requires 16-byte alignment at the end.
	int DataUnkCount = 0;
	int DataUnkOffset = 0x20;
};

enum class OutSGONodeType_t {
	raw,
	node,
	string,
	extra,
	invalid
};

struct OutSGONode_t {
	char data[12];
	tinyxml2::XMLElement* xmlNode;
	OutSGONodeType_t type;

	int hasName; // 0 is no name
	std::string name;

	bool operator<(const OutSGONode_t& other) const {
		return name < other.name;
	}
};

struct OutSGONodeNameOffset_t {
	std::wstring name;
	DataRelativeOffset_t ofs;
};

class SGO
{
public:
	void Read( const std::wstring& path );
	void ReadData(const std::vector<char>& buffer, tinyxml2::XMLElement* header, tinyxml2::XMLElement* xmlHeader);
	void Read4BytesData(bool big_endian, unsigned char* seg, const std::vector<char>& buffer, int position);
	void ReadSGOHeader(bool big_endian, const std::vector<char>& buffer);
	void ReadSGONode(bool big_endian, const std::vector<char>& buffer, int nodepos,
					std::vector<SGONode>& datanode, int i,
					tinyxml2::XMLElement*& xmlNode, tinyxml2::XMLElement* header, tinyxml2::XMLElement* xmlHeader);

	// Write
	void Write(const std::wstring& path, tinyxml2::XMLNode* header);
	std::vector< char > WriteData(tinyxml2::XMLElement* mainData, tinyxml2::XMLNode* header);

	void GetNodeExtraData(tinyxml2::XMLElement* entry, int& nodePtrNum);
	SGOExtraData GetExtraData(tinyxml2::XMLElement* entry, std::string dataName, tinyxml2::XMLNode* header);
	SGOExtraData GetNodeData(tinyxml2::XMLElement* entry, int size, int pos, std::vector< char > & NodeBytes);
	SGOExtraData GetNodeName(tinyxml2::XMLElement* entry, int pos, int NodeIndex);

	// new write
	void WriteData_WriteINT32LE(char* pdata, INT32 value);
	OutSGONode_t WriteData_GetNodeData(tinyxml2::XMLElement* entry, int needName);
	void WriteData_ProcessData(OutSGONode_t& in, int inPos, std::vector<char>& outBuffer, int curBufferPos, tinyxml2::XMLNode* header);

private:
	//std::map< std::wstring, SGONode * > node;

	std::vector< std::string > SubDataGroup;

	int DataNodeCount = 0;
	int DataNodeOffset = 0;
	int DataNameCount = 0;
	int DataNameOffset = 0;
	int DataUnkCount = 0;
	int DataUnkOffset = 0;
	// write
	std::vector< std::string > NodeString;
	std::vector< SGONodeName > NodeWString;
	int WstrPos = 0;
	std::vector< SGOExtraData > ExtraData;
	std::vector< int > ExtraDataPos;
	std::unordered_map<std::string, size_t> StringMap;
	std::unordered_map<std::wstring, size_t> WStringMap;

	// new write
	// about string list
	std::vector<DataRelativeOffset_t> NullString;
	std::vector<RelativeOffsetSet_WString_t> v_OfsString;
	WStringToIntMap map_string;
	// about extra data
	struct RelativeOffsetSet_ExtraData_t : RelativeOffsetSet_Data_t {
		tinyxml2::XMLElement* xmlNode;
	};
	std::vector<RelativeOffsetSet_ExtraData_t> v_OfsExtra;
	StringToIntMap map_extra;

	//std::vector<char> v_wstring;
};
