#pragma once
#include "include/tinyxml2.h"
#include "include/half.hpp"
#include "HashMap.h"
#include <xmmintrin.h>

// this is EDF6's canm

class CANM6 {
public:
	struct inHeader_t
	{
		int header;
		int version;
		int AnmDataCount;
		int AnmDataOffset;
		int KeyframeCount;
		int KeyframeOffset;
		int BoneCount;
		int BoneOffset;
	};

	struct inAnimationData_t {
		int pad0;
		int nameOffset;
		float time;
		float speed;
		// need keyframes +1
		int keyframe;
		// is animated bones
		int BoneCount;
		int BoneOffset;
	};

	struct inBoneData_t {
		// they are all indexes
		INT16 bone;
		INT16 position;
		INT16 rotation;
		INT16 scaling;
	};

	struct inKeyframeData_t {
		__m128 initial;
		__m128 velocity;
		int dataOffset;
		// determine type of keyframe data
		int type;
		// need keyframes +1
		int keyframe;
		int pad2C;
	};

	struct canmKeyframeData_t {
		inKeyframeData_t bin;
		// Own position in the input
		intptr_t CurPos;
		// Keyframe data position in the input
		intptr_t DataPos;
	};

	// out
	struct outKeyframeData_t {
		std::vector< char > out;
		std::vector< char > outKF;
		int selfOffset;
		// to check is same
		int type; // 3 is quaternion
		int keyframe;
	};

	struct outAnimationData_t {
		std::wstring name;
		std::vector< inBoneData_t > boneData;
		inAnimationData_t raw;
	};

	struct updateDataOffset_t {
		int pos, offset;
	};


	void ReadData(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader);
	void ReadAnimationData(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader);
	void ReadAnimationDataWriteKeyFrame(const std::vector<char>& buffer, tinyxml2::XMLElement* node, int num);
	void ReadKeyframeDataText(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlNode, const canmKeyframeData_t& Keyframe);
	void ReadKeyframeData(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader);
	void ReadBoneListData(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader);

	void ReadKeyframeCreateVector4Text(tinyxml2::XMLElement* xmlNode, const float* value);

	// write
	std::vector<char> WriteData(tinyxml2::XMLElement* Data);
	int WriteWideStringData(const std::string& in);
	int WriteWideStringToBuffer(const std::wstring& in);
	outAnimationData_t WriteAnimationData(tinyxml2::XMLElement* data);
	int WriteAnimationKeyFrame(tinyxml2::XMLElement* data);
	__m128 WriteKeyframeCreateVector4(tinyxml2::XMLElement* data);
	std::vector<char> WriteAnimationKeyFrameData(tinyxml2::XMLElement* Data, int* keyframe, int type);

private:
	inHeader_t header;

	std::vector< std::string > BoneList;
	std::vector< std::wstring > WBoneList;

	std::vector< canmKeyframeData_t > v_KeyframeData;

	// out 
	WStringToIntMap map_BoneList;
	std::vector<int> v_BoneList;
	std::vector<char> v_wstirngData;
	std::vector< outAnimationData_t > v_AnmData;
	std::vector< outKeyframeData_t > v_KFData;
	//std::vector<char> v_outKeyframe;
};
