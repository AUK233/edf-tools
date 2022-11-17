#pragma once
#include "include/tinyxml2.h"
#include "include/half.hpp"

class CANM
{
public:
	//void Read(std::wstring path);
	void ReadData(std::vector<char> buffer, tinyxml2::XMLElement* header);
	void ReadAnimationData(tinyxml2::XMLElement* header, std::vector<char> buffer);
	void ReadAnimationKeyData(tinyxml2::XMLElement* header, std::vector<char> buffer);
	void ReadBoneListData(tinyxml2::XMLElement* header, std::vector<char> buffer);

private:
	int i_AnmDataCount = 0;
	int i_AnmDataOffset = 0;
	int i_AnmPointCount = 0;
	int i_AnmPointOffset = 0;
	int i_BoneCount = 0;
	int i_BoneOffset = 0;

};