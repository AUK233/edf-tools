#pragma once
#include "include/tinyxml2.h"

struct CANMAnmKeyframe
{
	UINT16 vf[3];
	std::vector< char > bytes;
};

struct CANMAnmKey
{
	short vi[2];
	float vf[6];
	std::vector< CANMAnmKeyframe > kf;
	// when to CANM
	int pos;
	int offset;
	std::vector< char > bytes;
};

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
	// old
	void ReadAnimationPointData(tinyxml2::XMLElement* header, std::vector<char> buffer);
	// now
	void ReadAnimationData(tinyxml2::XMLElement* header, std::vector<char> buffer);
	void ReadAnimationDataWriteKeyFrame(tinyxml2::XMLElement* node, int num);
	void ReadBoneListData(tinyxml2::XMLElement* header, std::vector<char> buffer);
	CANMAnmKey ReadAnimationFrameData(std::vector<char> buffer, int pos);
	// faster with the pointer version
	CANMAnmKey ReadAnimationFrameData(std::vector<char> *buf, int pos, int mask);

	void Write(const std::wstring& path);
	std::vector< char > WriteData(tinyxml2::XMLElement* Data);
	CANMAnmPoint WriteAnmPoint(tinyxml2::XMLElement* data, std::vector<char>* bytes);
	CANMAnmData WriteAnmData(tinyxml2::XMLElement* data, std::vector<char>* bytes);
	std::vector< char > WriteBoneList(int in);
	// new
	CANMAnmData WriteAnimationData(tinyxml2::XMLElement* data, std::vector<char>* bytes);
	short WriteAnimationKeyFrame(tinyxml2::XMLElement* data);

private:
	int i_AnmDataCount = 0;
	int i_AnmDataOffset = 0;
	int i_AnmPointCount = 0;
	int i_AnmPointOffset = 0;
	int i_BoneCount = 0;
	int i_BoneOffset = 0;

	std::vector< std::string > BoneList;
	std::vector< std::wstring > WBoneList;

	std::vector< CANMAnmKey > v_AnmKey;

	std::vector< CANMAnmPoint > v_AnmPoint;
	std::vector< CANMAnmData > v_AnmData;

};