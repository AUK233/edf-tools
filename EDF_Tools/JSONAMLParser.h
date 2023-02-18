#pragma once

//Map is implied
#include <map>

struct JSONAMLValue
{
	//Todo: More refined system
	JSONAMLValue( ){};

	JSONAMLValue( std::wstring value )
	{
		val = value.c_str();
		subNodeType = 0;
	};

	std::wstring val;
	unsigned char subNodeType;
};

struct JSONAMLNode
{
	JSONAMLNode( std::wstring nodeText );
	std::wstring GetValue( std::wstring key );

	std::map< std::wstring, std::unique_ptr<JSONAMLValue> > values;
};

struct JSONAMLValueArr : public JSONAMLValue
{
	JSONAMLValueArr( std::wstring parseStrn );

	JSONAMLNode * LoopUpNode( std::wstring key, std::wstring value, bool caseSensitive = false );

	std::vector< std::unique_ptr<JSONAMLNode> > nodes;
};

struct JSONAMLValueSubnode : public JSONAMLValue
{
	JSONAMLValueSubnode( std::wstring parseStrn );

	std::unique_ptr<JSONAMLNode> node;
};

class CJSONAMLParser
{
public:
	CJSONAMLParser( std::wstring filePath );

	JSONAMLValueArr * GetValueAsArrNode( JSONAMLNode * node, std::wstring value );
	JSONAMLNode * FindNode( std::wstring key, std::wstring value, bool caseSensitive = false, JSONAMLNode *root = NULL );

	std::wstring SearchTest( );

protected:
	std::unique_ptr<JSONAMLNode> root;
};