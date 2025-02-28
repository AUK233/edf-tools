#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>

#include <iostream>
#include <locale>
#include <locale.h>
#include "util.h"
#include "RMPA.h"

#define TYPE_HEADER_SIZE 0x20

int CRMPA::Read( const std::wstring& path )
{
	std::ifstream file( path + L".RMPA", std::ios::binary | std::ios::ate);

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	if( file.read(buffer.data(), size) )
	{
		unsigned char seg[4];

		int position = 0;
		//position = 0x100;

		Read4BytesReversed( seg, buffer, position );

		bool validHeader = false;
		if( seg[0] == 0x00 && seg[1] == 0x50 && seg[2] == 0x4D && seg[3] == 0x52 )
			validHeader = true;

		if( !validHeader )
		{
			std::wcout << L"BAD FILE\n";
			file.close();
			return -1;
		}

		//std::wofstream output("RMPA_Data.txt", std::ios::binary | std::ios::out | std::ios::ate );

		//const unsigned long MaxCode = 0x10ffff;
		//const std::codecvt_mode Mode = std::generate_header;
		//std::locale utf16_locale(output.getloc(), new std::codecvt_utf16<wchar_t, MaxCode, Mode>);
		//output.imbue(utf16_locale);

		//Parse the header

		//0x8: Bool denoting if RMPA has route data
		position = 0x8;
		Read4BytesReversed( seg, buffer, position );
		hasRoutes = GetIntFromChunk( seg );
		std::wcout << L"Has Routes: " + ToString( hasRoutes ) + L"\n";

		position = 0xc;
		Read4BytesReversed( seg, buffer, position );
		routePos = GetIntFromChunk( seg );

		//0x10: Bool denoting if RMPA has shape data
		position = 0x10;
		Read4BytesReversed( seg, buffer, position );
		hasShapes = GetIntFromChunk( seg );
		std::wcout << L"Has Shapes: " + ToString( hasShapes ) + L"\n";

		position = 0x14;
		Read4BytesReversed( seg, buffer, position );
		shapePos = GetIntFromChunk( seg );

		//0x18: Bool denoting if RMPA has camera data
		position = 0x18;
		Read4BytesReversed( seg, buffer, position );
		hasCamData = GetIntFromChunk( seg );
		std::wcout << L"Has Camera Data: " + ToString( hasCamData ) + L"\n";

		position = 0x1C;
		Read4BytesReversed( seg, buffer, position );
		camPos = GetIntFromChunk( seg );

		//0x18: Bool denoting if RMPA has spawnpoint data
		position = 0x20;
		Read4BytesReversed( seg, buffer, position );
		hasSpawnpoints = GetIntFromChunk( seg );
		std::wcout << L"Has Spawnpoint Data: " + ToString( hasSpawnpoints ) + L"\n";

		position = 0x24;
		Read4BytesReversed( seg, buffer, position );
		spawnPos = GetIntFromChunk( seg );

		//Read spawnpoints:
		//ReadSpawnpoints( buffer );

		ReadRoutes( buffer );

		//Lets do something with our data:
		std::wstring input;
		std::wcin >> input;
	}
}

void CRMPA::ReadRoutes( const std::vector<char>& buffer )
{
	unsigned char seg[4];

	int position = routePos;
	Read4BytesReversed( seg, buffer, position );
	int numSubheaders = GetIntFromChunk( seg );
	std::wcout << L"Enumerators: " + ToString( numSubheaders ) + L"\n";

	position += 0x4;
	Read4BytesReversed( seg, buffer, position );
	int ofs = GetIntFromChunk( seg );
	std::wcout << L"Enumerators 1 pos: " + ToString( ofs ) + L"\n";

	//Subheader
	for( int i = 0; i < numSubheaders; i++ )
	{
		std::wcout << L"Enumerator " + ToString( i ) + L":\n";

		position = spawnPos + ofs + 0x8;
		Read4BytesReversed( seg, buffer, position );
		int endOfs = GetIntFromChunk( seg );
		std::wcout << L"Enumerator ends at: " + ToString( endOfs ) + L"\n";

		position = spawnPos + ofs + 0x18;
		Read4BytesReversed( seg, buffer, position );
		int dataCount = GetIntFromChunk( seg );
		std::wcout << L"Number of data in Enumerator " + ToString( i ) + L": " + ToString( dataCount ) + L"\n";

		position = spawnPos + ofs + 0x1C;
		Read4BytesReversed( seg, buffer, position );
		int num = GetIntFromChunk( seg );
		std::wcout << L"Data should begin at: " + ToString( num ) + L"\n";

		int clusterPos = spawnPos + ofs + num;

		//Read nodes
		for( int j = 0; j < dataCount; j++ )
		{
			routes.push_back( ReadRoute( clusterPos, buffer ) );

			std::wcout << L"Route: Name:" + routes.back( ).name + L" ";
			std::wcout << L"position in route:" + ToString( routes.back( ).number ) + L" ";
			std::wcout << L"ID:" + ToString( routes.back( ).ID ) + L" ";
			//std::wcout << L"Position: " + ToString( spawnPoints.back( ).x ) + L" " + ToString( spawnPoints.back( ).y ) + L" " + ToString( spawnPoints.back( ).z ) + L" ";
			//std::wcout << L"Angles: " + ToString( spawnPoints.back( ).pitch ) + L" " + ToString( spawnPoints.back( ).yaw ) + L" " + ToString( spawnPoints.back( ).roll ) + L" ";
			//std::wcout << L"\n";

			clusterPos += 0x3C;
		}

		ofs += 0x20;
	}
}

void CRMPA::ReadSpawnpoints( const std::vector<char>& buffer )
{
	unsigned char seg[4];

	int position = spawnPos;
	Read4BytesReversed( seg, buffer, position );
	int numSubheaders = GetIntFromChunk( seg );
	std::wcout << L"Enumerators: " + ToString( numSubheaders ) + L"\n";

	position += 0x4;
	Read4BytesReversed( seg, buffer, position );
	int ofs = GetIntFromChunk( seg );
	std::wcout << L"Enumerators 1 pos: " + ToString( ofs ) + L"\n";

	//Subheader
	for( int i = 0; i < numSubheaders; i++ )
	{
		std::wcout << L"Enumerator " + ToString( i ) + L":\n";

		position = spawnPos + ofs + 0x8;
		Read4BytesReversed( seg, buffer, position );
		int endOfs = GetIntFromChunk( seg );
		std::wcout << L"Enumerator ends at: " + ToString( endOfs ) + L"\n";

		position = spawnPos + ofs + 0x18;
		Read4BytesReversed( seg, buffer, position );
		int dataCount = GetIntFromChunk( seg );
		std::wcout << L"Number of data in Enumerator " + ToString( i ) + L": " + ToString( dataCount ) + L"\n";

		position = spawnPos + ofs + 0x1C;
		Read4BytesReversed( seg, buffer, position );
		int num = GetIntFromChunk( seg );
		std::wcout << L"Data should begin at: " + ToString( num ) + L"\n";

		int clusterPos = spawnPos + ofs + num;

		//Read nodes
		for( int j = 0; j < dataCount; j++ )
		{
			spawnPoints.push_back( ReadSpawnpoint( clusterPos, buffer ) );

			//std::wcout << spawnPoints.back( ).
			std::wcout << L"Spawnpoint: Name:" + spawnPoints.back( ).name + L" ";
			std::wcout << L"ID?:" + ToString( spawnPoints.back( ).num ) + L" ";
			std::wcout << L"Position: " + ToString( spawnPoints.back( ).x ) + L" " + ToString( spawnPoints.back( ).y ) + L" " + ToString( spawnPoints.back( ).z ) + L" ";
			std::wcout << L"Angles: " + ToString( spawnPoints.back( ).pitch ) + L" " + ToString( spawnPoints.back( ).yaw ) + L" " + ToString( spawnPoints.back( ).roll ) + L" ";
			std::wcout << L"\n";

			clusterPos += 0x40;
		}

		ofs += 0x20;
	}
}

RMPASpawnPoint CRMPA::ReadSpawnpoint( int pos, const std::vector<char>& buffer )
{
	unsigned char seg[] = { 0x0, 0x0, 0x0, 0x0 };

	int position = pos;
	Read4BytesReversed( seg, buffer, position );
	int unknown1 = GetIntFromChunk( seg );

	position += 0x4;
	Read4BytesReversed( seg, buffer, position );
	int unknown2 = GetIntFromChunk( seg );

	//0x8 data
	position += 0x4;
	Read4BytesReversed( seg, buffer, position );
	int id = GetIntFromChunk( seg );

	float vecPos[3];
	for( int i = 0; i < 3; i++ ) //+0xC
	{
		position += 4;
		Read4Bytes( seg, buffer, position );

		float f;
		memcpy( &f, &seg, sizeof( f ) );
		vecPos[i] = f;
	}

	float vecAng[3];
	for( int i = 0; i < 3; i++ ) //+0xc
	{
		position += 4;
		Read4Bytes( seg, buffer, position );

		float f;
		memcpy( &f, &seg, sizeof( f ) );
		vecAng[i] = f;
	}

	//0x20
	position += 0x4;
	Read4BytesReversed( seg, buffer, position );
	int unknown3 = GetIntFromChunk( seg );

	//float f;
	//memcpy( &f, &seg, sizeof( f ) );

	position += 0x4;
	Read4BytesReversed( seg, buffer, position );
	int unknown4 = GetIntFromChunk( seg );
	position += 0x4;
	Read4BytesReversed( seg, buffer, position );
	int unknown5 = GetIntFromChunk( seg );
	position += 0x4;
	Read4BytesReversed( seg, buffer, position );
	int unknown6 = GetIntFromChunk( seg );

	//Node name:
	position += 0x4;
	Read4BytesReversed( seg, buffer, position );
	int num = GetIntFromChunk( seg );
	//num++;
	std::wstring nodeName = ReadUnicode( buffer, pos + num, true );

	position += 0x4;
	Read4BytesReversed( seg, buffer, position );
	int unknown7 = GetIntFromChunk( seg );
	position += 0x4;
	Read4BytesReversed( seg, buffer, position );
	int unknown8 = GetIntFromChunk( seg );
	
	RMPASpawnPoint point = RMPASpawnPoint( nodeName, id, vecPos[0], vecPos[1], vecPos[2], vecAng[0], vecAng[1], vecAng[2] );

	//Name offset tracking data:
	point.nameoffset = num;

	//Unknown data
	point.unknown[0] = unknown1;
	point.unknown[1] = unknown2;
	point.unknown[2] = unknown3;
	point.unknown[3] = unknown4;
	point.unknown[4] = unknown5;
	point.unknown[5] = unknown6;
	point.unknown[6] = unknown7;
	point.unknown[7] = unknown8;

	return point;
}

RMPARoute CRMPA::ReadRoute( int pos, const std::vector<char>& buffer )
{
	unsigned char seg[] = { 0x0, 0x0, 0x0, 0x0 };

	int position = pos;
	Read4BytesReversed( seg, buffer, position );
	int number = GetIntFromChunk( seg );

	position += 0x4;
	Read4BytesReversed( seg, buffer, position );
	int unknown1 = GetIntFromChunk( seg );

	position += 0x4;
	Read4BytesReversed( seg, buffer, position );
	int ofsNextWaypointData = GetIntFromChunk( seg );

	position += 0x4;
	Read4BytesReversed( seg, buffer, position );
	int unknown2 = GetIntFromChunk( seg );

	position += 0x4;
	Read4BytesReversed( seg, buffer, position );
	int ofsSGO = GetIntFromChunk( seg );

	//Position?
	float vecPos[3];
	for( int i = 0; i < 3; i++ ) //+0xC
	{
		position += 4;
		Read4Bytes( seg, buffer, position );

		float f;
		memcpy( &f, &seg, sizeof( f ) );
		vecPos[i] = f;
	}

	position += 0x4;
	Read4BytesReversed( seg, buffer, position );
	int ofsSGO2 = GetIntFromChunk( seg );

	position += 0x4;
	Read4BytesReversed( seg, buffer, position );
	int unknown3 = GetIntFromChunk( seg );

	//Node name:
	position = pos + 0x24;
	Read4BytesReversed( seg, buffer, position );
	int num = GetIntFromChunk( seg );
	//num++;
	std::wstring nodeName = ReadUnicode( buffer, pos + num, true );

	RMPARoute route = RMPARoute( number, nodeName );

	route.ID = unknown3;
	//route.

	return route;
}

void RMPA6::Read(const std::wstring& path)
{
	std::ifstream file(path + L".rmpa", std::ios::binary | std::ios::ate | std::ios::in);

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	if (file.read(buffer.data(), size))
	{
		// create xml
		tinyxml2::XMLDocument xml;
		xml.InsertFirstChild(xml.NewDeclaration());
		tinyxml2::XMLElement* xmlHeader = xml.NewElement("RMPA");
		xml.InsertEndChild(xmlHeader);
		xmlHeader->SetAttribute("version", "6");

		// read data
		ReadHeader(buffer);
		ReadPointNode(buffer, xmlHeader);

		std::string outfile = WideToUTF8(path) + "_RMPA.xml";
		xml.SaveFile(outfile.c_str());
	}

	//Clear buffers
	buffer.clear();
	file.close();
}

void RMPA6::ReadHeader(const std::vector<char>& buffer)
{
	IsBigEndian = 0;
	if (*(int*)&buffer[0] == 1380798464) {
		IsBigEndian = 1;
		int* temp = (int*)&header;
		static_assert(sizeof(inHeader_t) == 12 * 4, "size is error.");
		for (int i = 0; i < 12; i++) {
			temp[i] = ReadInt32(&buffer[i * 4], 1);
		}
	}
	else {
		memcpy(&header, &buffer[0], 0x30);
	}
}

void RMPA6::ReadNode(const std::vector<char>& buffer, int pos, inNode_t* pNode)
{
	if (IsBigEndian) {
		int* temp = (int*)pNode;
		static_assert(sizeof(inNode_t) == 8 * 4, "size is error.");
		int curpos = pos;
		for (int i = 0; i < 8; i++) {
			temp[i] = ReadInt32(&buffer[curpos], 1);
			curpos += 4;
		}
	}
	else {
		memcpy(pNode, &buffer[pos], 0x20);
	}
}

void RMPA6::ReadSubNode(const std::vector<char>& buffer, int pos, inSubNode_t* pNode)
{
	if (IsBigEndian) {
		int* temp = (int*)pNode;
		static_assert(sizeof(inSubNode_t) == 5 * 4, "size is error.");
		int curpos = pos;
		for (int i = 0; i < 5; i++) {
			temp[i] = ReadInt32(&buffer[curpos], 1);
			curpos += 4;
		}
	}
	else {
		memcpy(pNode, &buffer[pos], 0x14);
	}
}

void RMPA6::ReadName(const std::vector<char>& buffer, int pos, tinyxml2::XMLElement* xmlNode)
{
	std::wstring wstr;
	std::string utf8str;
	if (*(INT16*)&buffer[pos]) {
		wstr = ReadUnicode(buffer, pos, IsBigEndian);
		utf8str = WideToUTF8(wstr);
		xmlNode->SetAttribute("name", utf8str.c_str());
	}
}

void RMPA6::ReadValue_SetFloat3(tinyxml2::XMLElement* xmlNode, const float* vf)
{
	xmlNode->SetAttribute("x", vf[0]);
	xmlNode->SetAttribute("y", vf[1]);
	xmlNode->SetAttribute("z", vf[2]);
}

void RMPA6::ReadPointNode(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader)
{
	inNode_t n_pointMain;
	int ofs_point = header.pointOffset;
	ReadNode(buffer, ofs_point, &n_pointMain);
	tinyxml2::XMLElement* xmlMainNode = xmlHeader->InsertNewChildElement("Point");
	ReadName(buffer, ofs_point + n_pointMain.nameOffset, xmlMainNode);
	// next is unknown value
	xmlMainNode->SetAttribute("pad0", n_pointMain.pad0);
	xmlMainNode->SetAttribute("pad4", n_pointMain.pad4);

	std::vector<inSubNode_t> v_PointNode(n_pointMain.subNodeCount);
	tinyxml2::XMLElement* xmlNode;
	tinyxml2::XMLElement* xmlData;
	tinyxml2::XMLElement* xmlValue;
	// get node offset
	int ofs_node = ofs_point + n_pointMain.subOffset;

	for (int i = 0; i < n_pointMain.subNodeCount; i++) {
		ReadSubNode(buffer, ofs_node, &v_PointNode[i]);
		xmlNode = xmlMainNode->InsertNewChildElement("Node");
		ReadName(buffer, ofs_node + v_PointNode[i].nameOffset, xmlNode);
		// next is unknown value
		xmlNode->SetAttribute("pad0", v_PointNode[i].pad0);
		xmlNode->SetAttribute("pad4", v_PointNode[i].pad4);

		int SubNodeCount = v_PointNode[i].subNodeCount;
		std::vector<inPointNode_t> v_Node(SubNodeCount);
		// get data offset
		int ofs_data = ofs_node + v_PointNode[i].subOffset;

		for (int j = 0; j < SubNodeCount; j++) {
			ReadPointNode(buffer, ofs_data, &v_Node[j]);
			xmlData = xmlNode->InsertNewChildElement("Data");
			ReadName(buffer, ofs_data + v_Node[j].nameOffset, xmlData);
			xmlValue = xmlData->InsertNewChildElement("position");
			ReadValue_SetFloat3(xmlValue, v_Node[j].pos1);
			xmlValue = xmlData->InsertNewChildElement("orientation");
			ReadValue_SetFloat3(xmlValue, v_Node[j].pos2);
			// next is unknown value
			xmlData->SetAttribute("pad0", v_Node[j].pad0);
			xmlData->SetAttribute("pad10", v_Node[j].pad10);
			xmlData->SetAttribute("pad20", v_Node[j].pad20[0]);
			xmlData->SetAttribute("pad24", v_Node[j].pad20[1]);
			xmlData->SetAttribute("pad2C", v_Node[j].pad2C[0]);
			xmlData->SetAttribute("pad30", v_Node[j].pad2C[1]);
			//
			ofs_data += 0x34;
		}

		ofs_node += 0x14;
	}
}

void RMPA6::ReadPointNode(const std::vector<char>& buffer, int pos, inPointNode_t* pNode)
{
	if (IsBigEndian) {
		int* temp = (int*)pNode;
		static_assert(sizeof(inPointNode_t) == 13 * 4, "size is error.");
		int curpos = pos;
		for (int i = 0; i < 13; i++) {
			temp[i] = ReadInt32(&buffer[curpos], 1);
			curpos += 4;
		}
	}
	else {
		memcpy(pNode, &buffer[pos], 0x34);
	}
}
