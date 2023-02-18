#pragma once
#include <map>

struct SGONode
{
	SGONode( int value ){ type = 1; ivalue = value; };
	SGONode( float value ){	type = 2; fvalue = value; };
	SGONode( std::wstring value ){ type = 3; strvalue = value; };
	SGONode( std::vector< SGONode * > value ){ type = 0; structvalue = value; };

	int type;

	//Value types
	int ivalue;
	float fvalue;
	std::wstring strvalue;
	std::vector< SGONode * > structvalue;
};

class SGO
{
public:
	void Read( std::wstring path );
	void ParseFromData( std::vector< char > in );

	SGONode *ParseNode( int position );
	std::vector< SGONode * > ParseStruct( int position, int size );

	//Writing SGO from nodal data.
	void Write();
	char * GenerateHeader();

private:
	std::vector< char > m_vecRawData;
	std::map< std::wstring, SGONode * > root;

	int m_iNumVariables;
	bool m_bVerbose;
};