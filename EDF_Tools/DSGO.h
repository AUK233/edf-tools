#pragma once
#include <map>
#include "include/tinyxml2.h"

//Value types
//std::vector< DSGONode* > ptr;
//std::wstring str;
//std::vector< char > data;

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
		uintptr_t pos;
	};

	void ReadData(const std::vector<char>& buffer, tinyxml2::XMLElement* header, tinyxml2::XMLElement* xmlHeader);
	void ReadDSGONode(bool big_endian, const std::vector<char>& buffer, int nodepos,
						const std::vector<DSGOStandardNode>& datanode, int SN,
						tinyxml2::XMLElement* header, tinyxml2::XMLElement* xmlHeader);
	void ReadDSGONodeSetNodeName(const std::vector<DSGONameTalbe>& nameList, int SN, tinyxml2::XMLElement*& xmlNode);

private:
	int DataNodeCount = 0; // header + 0x8
	int DataNodeOffset = 0; // header + 0xC
	std::vector< std::string > SubDataGroup;
};
