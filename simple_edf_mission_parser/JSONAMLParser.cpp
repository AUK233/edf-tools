#include "stdafx.h"

#include <vector>
#include <string>
#include <codecvt>
#include <iostream>
#include <fstream>
#include "JSONAMLParser.h"
#include "util.h"

//Whoo boy, this is a mess. I need to comment this eventually.

JSONAMLNode::JSONAMLNode( std::wstring node )
{
	int index = 0;
	int arrDepth = 0;
	int context = 0;
	bool valueIsArray = 0;
	bool isKey = true;

	std::wstring curStrn;
	std::wstring curKey;
	std::wstring curValue;

	while( index < node.size( ) )
	{
		if( node[index] == ':' && isKey )
		{
			isKey = false;
			curKey = curStrn;
			curStrn.clear( );
		}
		else if( node[index] == '[' )
		{
			if( !isKey && arrDepth == 0 )
			{
				valueIsArray = true;
			}
			
			if( arrDepth > 0 )
				curValue += node[index];

			++arrDepth;
		}
		else if( node[index] == ']' && !isKey )
		{
			--arrDepth;

			if( arrDepth == 0 )
			{
				values[curKey] = std::make_unique<JSONAMLValueArr>( curValue );
				curValue.clear( );
			}
			else
				curValue += node[index];
		}
		else if( node[index] == '{' && !isKey && !valueIsArray )
		{
			if( context > 0 )
				curValue += node[index];
			++context;
		}
		else if( node[index] == '}' && !isKey && !valueIsArray )
		{
			--context;
			if( context == 0 )
			{
				values[curKey] = std::make_unique<JSONAMLValueSubnode>( curValue );
				curStrn.clear( );
				curValue.clear( );

				valueIsArray = true; //Ignore existing behavour.
			}
			else
				curValue += node[index];
		}
		else if( node[index] == ',' && arrDepth == 0 && context == 0 )
		{
			if( !valueIsArray )
			{
				values[curKey] = std::make_unique<JSONAMLValue>( curStrn.c_str( ) );
			}

			valueIsArray = false;
			isKey = true;
			curStrn.clear( );
		}
		else
		{
			if( (arrDepth > 0 || context > 0) && !isKey )
			{
				curValue += node[index];
			}
			curStrn += node[index];
		}
		++index;
	}
}

std::wstring JSONAMLNode::GetValue( std::wstring key )
{
	if( DetermineType( values[key]->val ) == T_STRING )
	{
		std::wstring returnString;
		returnString = values[key]->val.substr( 1, values[key]->val.size( ) - 2 );

		return returnString;
	}
	return values[key]->val;
}

CJSONAMLParser::CJSONAMLParser( std::wstring filePath )
{
	std::wstring rawString = L"root:" + ReadFile( filePath.c_str() );

	std::wstring proccess = KillWhitespace( rawString );
	FindAndReplaceAll( proccess, L"\r", L"" );
	FindAndReplaceAll( proccess, L"\n", L"" );

	root = std::make_unique< JSONAMLNode >( proccess );
}

JSONAMLValueArr * CJSONAMLParser::GetValueAsArrNode( JSONAMLNode * node, std::wstring value )
{
	return (JSONAMLValueArr *)( node->values[value].get( ) );
}

JSONAMLNode * CJSONAMLParser::FindNode( std::wstring key, std::wstring value, bool caseSensitive, JSONAMLNode * iroot )
{
	if( iroot == NULL )
	{
		JSONAMLValueArr * val = (JSONAMLValueArr *)( root->values[L"root"].get( ) );
		JSONAMLNode * node = val->LoopUpNode( key, value, caseSensitive );

		if( node )
			return node;
		else
		{
			for( int i = 0; i < val->nodes.size( ); ++i )
			{
				JSONAMLNode *node = FindNode( key, value, caseSensitive, val->nodes[i].get( ) );
				if( node )
					return node;
			}
		}
	}
	else
	{
		for( int i = 0; i < iroot->values.size( ); ++i )
		{

		}
	}

	return nullptr;
}

std::wstring CJSONAMLParser::SearchTest( )
{
	JSONAMLNode * node = FindNode( L"representations", L"0x2E" );

	std::wstring output;
	output += node->GetValue(L"name").substr( 1, node->GetValue( L"name" ).size( ) - 2 );
	output += L"( ";

	int numArgs = GetValueAsArrNode( node, L"arguments" )->nodes.size( );
	for( int i = 0; i < numArgs; ++i )
	{
		std::wstring temp = GetValueAsArrNode( node, L"arguments" )->nodes[i]->values[L"type"]->val;
		temp = temp.substr( 1, temp.size( ) - 2 );
		output += temp + L" ";
		temp = GetValueAsArrNode( node, L"arguments" )->nodes[i]->values[L"name"]->val;
		temp = temp.substr( 1, temp.size( ) - 2 );
		output += temp + L", ";
	}

	output.pop_back( );
	output.pop_back( );
	output += L" );";

	return output;
	//return std::wstring( );
}

JSONAMLValueArr::JSONAMLValueArr( std::wstring parseStrn )
{
	val = L"ARRAY";
	subNodeType = 2;

	int index = 0;
	int depth = 0;

	std::wstring curStrn;
	std::wstring curKey;
	std::wstring curValue;

	while( index < parseStrn.size( ) )
	{
		if( parseStrn[index] == '{' )
		{
			if( depth > 0 )
			{
				curValue += parseStrn[index];
			}

			++depth;
		}
		else if( parseStrn[index] == '}' )
		{
			--depth;

			if( depth == 0 )
			{
				nodes.push_back( std::make_unique<JSONAMLNode>( curValue ) );
				curValue.clear( );
				curStrn.clear( );
			}
			else
				curValue += parseStrn[index];
		}
		else
		{
			if( depth > 0 )
			{
				curValue += parseStrn[index];
			}
			curStrn += parseStrn[index];
		}
		++index;
	}

	if( curStrn.size( ) > 0 && nodes.size() == 0 )
	{
		val = curStrn;
	}
}

JSONAMLNode * JSONAMLValueArr::LoopUpNode( std::wstring key, std::wstring value, bool caseSensitive )
{
	for( int i = 0; i < nodes.size( ); ++i )
	{
		if( nodes[i]->values[key] )
		{
			std::wstring compareValue( value.c_str() );
			std::wstring testedValue( nodes[i]->values[key]->val.c_str() );

			if( !caseSensitive )
			{
				compareValue = ConvertToLower( compareValue );
				testedValue = ConvertToLower( testedValue );
			}

			if( testedValue == compareValue )
				return nodes[i].get( );
		}
	}
	return nullptr;
}

JSONAMLValueSubnode::JSONAMLValueSubnode( std::wstring parseStrn )
{
	val = L"NODE";
	subNodeType = 1;

	node = std::make_unique<JSONAMLNode>( parseStrn );
}
