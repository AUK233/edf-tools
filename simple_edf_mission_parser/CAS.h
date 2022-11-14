#pragma once
#include "include/tinyxml2.h"

class CAS
{
public:
	void Read(std::wstring path);
	void ReadData(std::vector<char> buffer, tinyxml2::XMLElement* header);

	void ReadTControlData(tinyxml2::XMLElement* header, std::vector<char> buffer);
	void ReadVControlData(tinyxml2::XMLElement* header, std::vector<char> buffer);
	void ReadAnmGroupData(tinyxml2::XMLElement* header, std::vector<char> buffer);
	void ReadAnmGroupNodeData(std::vector<char> buffer, int ptrpos, tinyxml2::XMLElement* xmlptr);
	void ReadAnmGroupNodeDataPtrA(std::vector<char> buffer, tinyxml2::XMLElement* xmldata, int pos);
	void ReadAnmGroupNodeDataPtrB(std::vector<char> buffer, tinyxml2::XMLElement* xmldata, int pos);
	void ReadAnmGroupNodeDataPtrCommon(std::vector<char> buffer, tinyxml2::XMLElement* xmldata, int pos);
	void ReadAnmGroupNodeDataCommon(std::vector<char> buffer, tinyxml2::XMLElement* xmldata, int pos);
	void ReadBoneListData(tinyxml2::XMLElement* header, std::vector<char> buffer);

private:
	int CAS_Version;
	// CasDataCommon length
	int i_CasDCCount = 0;
	int CANM_Offset;
	int i_TControlCount = 0;
	int i_TControlOffset = 0;
	int i_VControlCount = 0;
	int i_VControlOffset = 0;
	int i_AnmGroupCount = 0;
	int i_AnmGroupOffset = 0;
	int i_BoneCount = 0;
	int i_BoneOffset = 0;
	int i_UnkCOffset = 0;

};