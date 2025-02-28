#pragma once
#include "include/tinyxml2.h"

//RMPA spawnpoint
struct RMPASpawnPoint
{
	RMPASpawnPoint( const std::wstring& nm, int inum, float ix, float iy, float iz, float p, float ya, float ro )
	{
		name = nm;
		x = ix;
		y = iy;
		z = iz;

		pitch = p;
		yaw = ya;
		roll = ro;

		num = inum;
	}

	int num;
	int unknown[8];

	std::wstring name;
	int nameoffset;

	float x;
	float y;
	float z;

	float pitch;
	float yaw;
	float roll;
};

struct RMPARoute
{
	RMPARoute( int num, std::wstring iname )
	{
		number = num;
		name = iname;
	}

	int number;
	int ofsNextNode;
	int ofsSGO;
	int ofsSGO2;
	int ID;

	std::wstring name;
	int nameOffset;
};

struct RMPAShapeSetup
{

};

struct RMPAShapeData
{

};

//RMPA Enumerator
struct RMPAEnumerator
{

};

class CRMPA
{
public:
	int Read( const std::wstring& path );

	void ReadSpawnpoints( const std::vector<char>& buffer );
	void ReadRoutes( const std::vector<char>& buffer );

	RMPASpawnPoint ReadSpawnpoint( int pos, const std::vector<char>& buffer );
	RMPARoute ReadRoute( int pos, const std::vector<char>& buffer );

private:
	std::vector< RMPASpawnPoint > spawnPoints;
	std::vector< RMPARoute > routes;

	//Header data
	bool hasRoutes;
	bool hasShapes;
	bool hasCamData;
	bool hasSpawnpoints;

	int spawnPos;
	int camPos;
	int routePos;
	int shapePos;
};

// this is EDF6's rmpa

class RMPA6
{
public:
	struct inHeader_t
	{
		int header;
		int version;
		int routeCount;
		int routeOffset;
		int shapeCount;
		int shapeOffset;
		int cameraCount;
		int cameraOffset;
		int pointCount;
		int pointOffset;
		BYTE pad28[8];
	};

	struct inNode_t
	{
		int pad0;
		int pad4;
		int nameOffset;
		int subNodeCount;
		int subOffset;
		BYTE pad14[12];
	};

	struct inSubNode_t
	{
		int pad0;
		int pad4;
		int nameOffset;
		int subNodeCount;
		int subOffset;
	};

	struct inPointNode_t
	{
		int pad0;
		float pos1[3];
		int pad10;
		float pos2[3];
		int pad20[2];
		int nameOffset;
		int pad2C[2];
	};

	void Read(const std::wstring& path);
	void ReadHeader(const std::vector<char>& buffer);
	void ReadNode(const std::vector<char>& buffer, int pos, inNode_t* pNode);
	void ReadSubNode(const std::vector<char>& buffer, int pos, inSubNode_t* pNode);
	void ReadName(const std::vector<char>& buffer, int pos, tinyxml2::XMLElement* xmlNode);
	void ReadValue_SetFloat3(tinyxml2::XMLElement* xmlNode, const float* vf);
	void ReadPointNode(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader);
	void ReadPointNode(const std::vector<char>& buffer, int pos, inPointNode_t* pNode);

private:
	inHeader_t header;
	int IsBigEndian;

};