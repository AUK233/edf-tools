#pragma once

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
