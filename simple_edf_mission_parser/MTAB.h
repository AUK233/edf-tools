#pragma once

struct MTABData
{
	int pos;
	int offset;
	// main data
	std::vector< char > bytes1;
	// sub data
	std::vector< char > bytes2;
};

struct MTABMainAction
{
	int pos;
	// used to mark required subobject offset
	int saofs;
	std::string str;
	std::wstring wstr;
	std::vector< char > bytes;
};

class MTAB
{
public:
	void Read(std::wstring path);
	void ReadData(std::vector<char> buffer, tinyxml2::XMLElement* header, tinyxml2::XMLElement* xmlHeader);
	void ReadMainActionData(std::vector<char>& buffer, int curpos, tinyxml2::XMLElement* xmlData, tinyxml2::XMLElement* xmlHeader);
	void ReadSubActionData(std::vector<char>& buffer, int curpos, tinyxml2::XMLElement* xmlData, tinyxml2::XMLElement* xmlHeader);
	void ReadNodeData(std::vector<char>& buffer, int curpos, tinyxml2::XMLElement* xmlData, tinyxml2::XMLElement* xmlHeader);

	// write
	void Write(std::wstring path, tinyxml2::XMLNode* header);
	std::vector< char > WriteData(tinyxml2::XMLElement* mainData, tinyxml2::XMLNode* header);
	MTABMainAction WriteMainAction(tinyxml2::XMLElement* data);
	MTABMainAction WriteSubAction(tinyxml2::XMLElement* data);
	MTABData WriteDataNode(tinyxml2::XMLElement* data);

private:
	int i_MainActionCount = 0;
	int i_MainActionOffset = 0;
	int i_SubActionCount = 0;

	std::vector< std::string > NameList;
	std::vector< std::wstring > WNameList;

	std::vector< MTABMainAction > v_MainAction;
	std::vector< MTABMainAction > v_SubAction;
	std::vector< MTABData > v_Data;
};