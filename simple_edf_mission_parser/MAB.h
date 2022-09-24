#pragma once

struct MABString
{
	std::wstring name;
	int pos;
};

struct MABFloatGroup
{
	float f[4];
	//to check for duplicates
	int pos;
	// write
	std::vector< char > bytes;
};

struct MABExtraData
{
	std::string name;
	int pos;
	std::vector< char > bytes;
};

struct MABData
{
	std::vector< char > bytes;
};

class MAB
{
public:
	void Read(std::wstring path);
	void ReadData(std::vector<char> buffer, tinyxml2::XMLElement* header, tinyxml2::XMLElement* xmlHeader);
	void ReadBoneData(std::vector<char>& buffer, int curpos, tinyxml2::XMLElement* xmlBone, tinyxml2::XMLElement* xmlHeader);
	void ReadBoneTypeData(int type, std::vector<char>& buffer, int ptrpos, tinyxml2::XMLElement* xmlBPtr);
	void Read4FloatData(tinyxml2::XMLElement* xmlNode, float* vf);
	void ReadExtraSGO(std::string& namestr, std::vector<char>& buffer, int pos, tinyxml2::XMLElement*& xmlHeader);
	void ReadAnimeData(std::vector<char>& buffer, int curpos, tinyxml2::XMLElement* xmlAnm, tinyxml2::XMLElement* xmlHeader);
	void ReadAnimeDataA(std::vector<char>& buffer, int pos, tinyxml2::XMLElement* xmlNode, tinyxml2::XMLElement* xmlHeader);

	void Write(std::wstring path, tinyxml2::XMLNode* header);
	std::vector< char > WriteData(tinyxml2::XMLElement* mainData, tinyxml2::XMLNode* header);
	void GetMABString(std::string namestr);
	void GetMABExtraDataName(tinyxml2::XMLElement* data);
	void GetMABFloatGroup(tinyxml2::XMLElement* entry3);

	MABExtraData GetExtraData(tinyxml2::XMLElement* entry, std::string dataName, tinyxml2::XMLNode* header, int pos);

	int GetMABStringOffset(std::string namestr);
	int GetMABExtraOffset(std::string namestr);

	MABData GetMABBoneData(tinyxml2::XMLElement* entry2, int ptrpos);
	MABData GetMABBonePtrData(tinyxml2::XMLElement* entry3);

	int GetMABBonePtrValue(tinyxml2::XMLElement* entry);

	MABData GetMABAnimeData(tinyxml2::XMLElement* entry2, int ptrpos, int nullpos);
	MABData GetMABAnimePtrData(tinyxml2::XMLElement* entry3, int nullpos);

private:
	short BoneCount = 0;
	short AnimationCount = 0;
	short FloatGroupCount = 0;
	short StringSize = 0;
	int BoneOffset = 0;
	int AnimationOffset = 0;
	int FloatGroupOffset = 0;
	int StringOffset = 0;

	std::vector< std::string > NodeString;
	std::vector< MABString > NodeWString;
	std::vector< MABFloatGroup > FloatGroup;
	std::vector< std::string > SubDataGroup;
	std::vector< MABExtraData > ExtraData;

	//only wrtie
	std::vector< MABData > boneData;
	std::vector< MABData > bonePtrData;
	std::vector< MABData > animeData;
	std::vector< MABData > animePtrData;
};