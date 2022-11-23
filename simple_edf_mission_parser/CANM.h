#pragma once
#include "include/tinyxml2.h"
#include "include/half.hpp"

struct CANMAnmPoint
{
	int pos;
	int offset;
	std::vector< char > bytes;
	bool haskey;
};

struct CANMAnmData
{
	int pos;
	std::wstring wstr;
	int offset;
	std::vector< char > bytes;
};

class CANM
{
public:
	void Read(std::wstring path);
	void ReadData(std::vector<char> buffer, tinyxml2::XMLElement* header);
	void ReadAnimationData(tinyxml2::XMLElement* header, std::vector<char> buffer);
	void ReadAnimationKeyData(tinyxml2::XMLElement* header, std::vector<char> buffer);
	void ReadBoneListData(tinyxml2::XMLElement* header, std::vector<char> buffer);

	void Write(const std::wstring& path);
	std::vector< char > WriteData(tinyxml2::XMLElement* Data);

	CANMAnmPoint WriteAnmPoint(tinyxml2::XMLElement* data, std::vector< char >* bytes);
	CANMAnmData WriteAnmData(tinyxml2::XMLElement* data, std::vector< char >* bytes);
	std::vector< char > WriteBoneList(int in);

private:
	int i_AnmDataCount = 0;
	int i_AnmDataOffset = 0;
	int i_AnmPointCount = 0;
	int i_AnmPointOffset = 0;
	int i_BoneCount = 0;
	int i_BoneOffset = 0;

	std::vector< std::string > BoneList;
	std::vector< std::wstring > WBoneList;

	std::vector< CANMAnmPoint > v_AnmPoint;
	std::vector< CANMAnmData > v_AnmData;

};