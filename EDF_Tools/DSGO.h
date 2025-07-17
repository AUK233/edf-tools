#pragma once
#include "include/tinyxml2.h"
#include "HashMap.h"


class DSGO;

struct DSGOType4Mapping_t
{
	int32_t number;
	char* name;
};


class DSGOoutBaseTable_t {
public:
	// do nothing
	virtual std::vector<char> WriteData(DSGO* p, int baseOffset) {
		return std::vector<char>();
	}
};

class DSGO
{
public:

	union DSGONodeValue {
		int64_t vi;
		double vf;
	};

	struct DSGONameTalbe
	{
		int32_t index;
		std::wstring wstr;
	};

	struct DSGOStandardNode
	{
		DSGONodeValue v;
		int64_t type;
		// used for reading
		uintptr_t pos;
		// detecting if it has been repeatedly read
		int32_t readCount;
		// if read by type4
		int32_t readByType4;
		// add the node's identifying name
		std::string nodeMark;
	};

	void ReadData(const std::vector<char>& buffer, tinyxml2::XMLElement* header, tinyxml2::XMLElement* xmlHeader);
	// Preprocess the node information to find the type4 loaded node.
	void PreReadDSGONode(bool big_endian, const std::vector<char>& buffer, std::vector<DSGOStandardNode>& datanode);
	//
	void ReadDSGONode(bool big_endian, const std::vector<char>& buffer, int nodepos,
						std::vector<DSGOStandardNode>& datanode, int SN,
						tinyxml2::XMLElement* header, tinyxml2::XMLElement* xmlHeader);
	//
	void ReadDSGONodeSetNodeName(const std::vector<DSGONameTalbe>& nameList, int SN, tinyxml2::XMLElement*& xmlNode);
	//
	void ReadDSGONodeType4(bool big_endian, const std::vector<char>& buffer, int nodepos,
						const std::vector<DSGOStandardNode>& datanode, int SN,
						tinyxml2::XMLElement*& xmlNode);

	// ==================================================================================================
	// write
	struct inHeader_t
	{
		int header;
		int unk0x4; // always 0x10
		int nodeCount; // need +1
		int nodeOffset; // should be 0x10
	};

	struct updateDataOffset_t {
		// pos is rosition to be written to
		// offset is rrite value
		int pos, offset;
	};

	struct inNode_t
	{
		DSGONodeValue v;
		int64_t type;
	};

	struct inPtrTable_t
	{
		int nameTableOffset, nameTableCount;
		// variable table is a group of node indexes
		int varTableOffset, varTableCount;
	};

	struct outNameTable_t
	{
		std::string name;
		int varIndex;
		// offset from string block
		int offset;
		// rewrite offset position, so index needs to be marked.

		bool operator<(const outNameTable_t& other) const {
			return name < other.name;
		}
	};

	struct outExtraData_t
	{
		std::string name;
		std::vector< char > bytes;
		int size;
	};


	void Write(const std::wstring& path, tinyxml2::XMLNode* header);
	std::vector< char > WriteData(tinyxml2::XMLElement* mainData, tinyxml2::XMLNode* header);

	int WriteData_WideString(std::string str);
	void WriteData_SetNodeIndex(const char* pName, int index);
	void WriteData_SetExtraData(tinyxml2::XMLElement* entry, tinyxml2::XMLNode* header);
	int WriteData_GetNodeIndex(const char* pName);

	// type 0
	int WriteData_GetValueData(tinyxml2::XMLElement* xmlNode);
	// type 1
	int WriteData_GetStringData(tinyxml2::XMLElement* xmlNode);
	// type 2
	int WriteData_GetExtraData(tinyxml2::XMLElement* xmlNode);
	int WriteData_GetExtraDataIndex(std::string str);
	// type 3
	int WriteData_GetPtrData(tinyxml2::XMLElement* xmlNode);
	// type 4
	int WriteData_GetMathData(tinyxml2::XMLElement* xmlNode);
	// only used to get index of an existing node
	int WriteData_GetReuseData(tinyxml2::XMLElement* xmlNode);

	std::vector<inNode_t> v_outNode;
	//
	std::vector<updateDataOffset_t> v_update_string;
private:
	int DataNodeCount = 0; // header + 0x8
	int DataNodeOffset = 0; // header + 0xC
	std::vector< std::string > SubDataGroup;

	// ==================================================================================================
	// write
	inHeader_t outHeader;

	// record node index
	StringToIntMap map_mark;
	// record type4 opcode
	StringToIntMap map_type4opcode;

	// about string list
	StringToIntMap map_string;
	std::vector<char> v_wstring;

	std::vector< DSGOoutBaseTable_t* > v_outPtrTable;

	std::vector< outExtraData_t > v_outExtraData;
	StringToIntMap map_outExtraData;

};


class DSGOoutPtrTable_t : public DSGOoutBaseTable_t
{
public:
	std::vector<char> WriteData(DSGO* p, int baseOffset) override;

	// ok, +0 is virtual function table
	int nodeIndex; // for record node index
	int pad;
	DSGO::inPtrTable_t table;
	std::vector< int > node;
	std::vector< DSGO::outNameTable_t > nameTable;
};

class DSGOoutExtraData_t : public DSGOoutBaseTable_t
{
public:
	std::vector<char> WriteData(DSGO* p, int baseOffset) override;

	// ok, +0 is virtual function table
	int nodeIndex; // for record node index
	int pad;
	std::vector<char> buffer;
};
