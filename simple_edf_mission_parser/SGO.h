#pragma once
#include <map>
#include "include/tinyxml2.h"

struct SGONode
{
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
	int id;
};

class SGO
{
public:
	void Read( std::wstring path );
	void ReadData(std::vector<char> buffer, tinyxml2::XMLElement* header, tinyxml2::XMLElement* xmlHeader);
	void Read4BytesData(bool big_endian, unsigned char* seg, std::vector<char> buffer, int position);
	void ReadSGOHeader(bool big_endian, std::vector<char> buffer);
	void ReadSGONode(bool big_endian, std::vector<char> buffer, int nodepos, std::vector<SGONode>& datanode, int i, tinyxml2::XMLElement*& xmlNode, tinyxml2::XMLElement* header, tinyxml2::XMLElement* xmlHeader);

	//Writing SGO from nodal data.
	//void Write();
	//char * GenerateHeader();

private:
	std::map< std::wstring, SGONode * > node;

	int DataNodeCount = 0;
	int DataNodeOffset = 0;
	int DataNameCount = 0;
	int DataNameOffset = 0;
	int DataUnkCount = 0;
	int DataUnkOffset = 0;

	int m_iNumVariables;
};
