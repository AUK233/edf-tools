#pragma once
#include "include/tinyxml2.h"

class RMPA6
{
public:
	struct inHeader_t
	{
		int header;
		int version;
		int routeCount, routeOffset;
		int shapeCount, shapeOffset;
		int cameraCount, cameraOffset;
		int pointCount, pointOffset;
		BYTE pad28[8];
	};

	// note: check 16-byte alignment at node end
	struct inSubNode_t
	{
		// always -1
		int pad0;
		//
		int nameSize, nameOffset;
		int subNodeCount, subOffset;
	};

	struct inNode_t : inSubNode_t
	{
		// 16-byte alignment
		int pad14[3];
	};

	// route node
	struct inWPNode_t : inSubNode_t
	{
		int waypointCount, waypointOffset;
		// 16-byte alignment
		int pad1c;
	};

	// route sub node has 0x18 bytes
	// check 16-byte alignment at node end
	struct inWPSubNode_t
	{
		// always -1
		int pad0;
		//
		int nameSize, nameOffset;
		// waypoint index used is stored here.
		int indexCount, indexOffset;
		// this is data start pos
		int dataStartOffset;
	};

	struct inRouteNode_t
	{
		int index; // probably useless, but the same as an index
		// this is linked waypoint
		int WPCount, WPOffset;
		int IndexID; // need to verify
		int nameSize, nameOffset;
		float pos[4]; // W maybe for alignment xmm
		// info's offset starts from here
		int infoCount, infoOffset;
	};

	struct outRouteNode_t {
		inRouteNode_t in;
		int pos;
		// it is marked as an index
		std::string str_node;
	};

	// note: check 16-byte alignment at end
	struct inShapeNode_t
	{
		// this is string
		int typeSize, typeOffset;
		//
		int nameSize, nameOffset;
		int IndexID;
		int dataCount, dataOffset;
		// info's offset starts from here
		int infoCount, infoOffset;
	};

	__declspec(align(16)) struct inShapeData_t
	{
		// W is used for alignment, it has no effect.
		float pos[4];
		float vec1[4];
		float vec2[4];
		float radius, height;
		// 16-byte aligned?
		UINT64 pad38;
	};

	// note: check 16-byte alignment at end
	struct inRMPAInfo_t
	{
		int titleOffset;
		int contentOffset;
	};

	// note: check 16-byte alignment at end
	struct inPointNode_t
	{
		int IndexID;
		float pos1[3];
		int pad10;
		float pos2[3];
		// another data block
		inSubNode_t data;
	};

	void Read(const std::wstring& path);

	void ReadHeader(const std::vector<char>& buffer);
	void ReadNode(const std::vector<char>& buffer, int pos, inNode_t* pNode);
	void ReadSubNode(const std::vector<char>& buffer, int pos, inSubNode_t* pNode);
	void ReadName(const std::vector<char>& buffer, int pos, tinyxml2::XMLElement* xmlNode);
	void ReadStringToXml(const std::vector<char>& buffer, int pos, tinyxml2::XMLElement* xmlNode, const char* name);
	void ReadValue_SetFloat3(tinyxml2::XMLElement* xmlNode, const float* vf);

	void ReadRouteNode(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader);
	void ReadRouteNode(const std::vector<char>& buffer, int pos, inWPNode_t* pNode);
	void ReadRouteNode(const std::vector<char>& buffer, int pos, inRouteNode_t* pNode);
	void ReadRouteSubNode(const std::vector<char>& buffer, int pos, inWPSubNode_t* pNode);

	void ReadShapeNode(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader);
	void ReadShapeNode(const std::vector<char>& buffer, int pos, inShapeNode_t* pNode);
	void ReadShapeNodeData(const std::vector<char>& buffer, int pos, inShapeData_t* pNode);

	void ReadPointNode(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader);
	void ReadPointNode(const std::vector<char>& buffer, int pos, inPointNode_t* pNode);

private:
	inHeader_t header;
	int IsBigEndian;
	int DataNodeCount;

};
