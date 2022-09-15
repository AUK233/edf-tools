#pragma once

struct MABFloatGroup
{
	float f[4];
	//to check for duplicates
	int pos;
	// write
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

private:
	short BoneCount = 0;
	short AnimationCount = 0;
	short FloatGroupCount = 0;
	short StringSize = 0;
	int BoneOffset = 0;
	int AnimationOffset = 0;
	int FloatGroupOffset = 0;
	int StringOffset = 0;

	std::vector< MABFloatGroup > FloatGroup;
	std::vector< std::string > SubDataGroup;

};