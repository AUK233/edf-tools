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
#include "SGO.h"
#include "Middleware.h"
#include "include/tinyxml2.h"

//Read data from SGO
void SGO::Read( std::wstring path )
{
	std::ifstream file(path + L".sgo", std::ios::binary | std::ios::ate | std::ios::in);

	std::streamsize size = file.tellg( );
	file.seekg( 0, std::ios::beg );

	std::vector<char> buffer( size );
	if( file.read( buffer.data( ), size ) )
	{
		// create xml
		tinyxml2::XMLDocument xml = new tinyxml2::XMLDocument();
		xml.InsertFirstChild(xml.NewDeclaration());
		tinyxml2::XMLElement* xmlHeader = xml.NewElement("EDFDATA");
		xml.InsertEndChild(xmlHeader);

		tinyxml2::XMLElement* xmlMain = xmlHeader->InsertNewChildElement("Main");
		xmlMain->SetAttribute("header", "SGO");

		ReadData(buffer, xmlMain, xmlHeader);
		/*
		std::string outfile = WideToUTF8(path) + "_DATA.xml";
		xml.SaveFile(outfile.c_str());
		*/
		tinyxml2::XMLPrinter printer;
		xml.Accept(&printer);
		auto xmlString = std::string{ printer.CStr() };

		std::wcout << UTF8ToWide(xmlString) + L"\n";
	}

	//Clear buffers
	buffer.clear( );
	file.close( );
}

void SGO::ReadData(std::vector<char>buffer, tinyxml2::XMLElement* header, tinyxml2::XMLElement* xmlHeader)
{
	bool big_endian = false;
	int position = 0;
	unsigned char seg[4];
	// read header
	memcpy(&seg, &buffer[0], 4U);
	if (seg[0] == 0x53 && seg[1] == 0x47 && seg[2] == 0x4f && seg[3] == 0x00)
	{
		big_endian = false;
	}
	else if (seg[3] == 0x53 && seg[2] == 0x47 && seg[1] == 0x4f && seg[0] == 0x00)
	{
		big_endian = true;
	}
	// read version
	int ver;
	position = 0x4;
	Read4BytesData(big_endian, seg, buffer, position);
	// memcpy is little endian
	memcpy(&ver, &seg, 4U);
	if (ver != 258)
		std::wcout << L"This is not a file for EDF 4.1 or 5!\n";

	// read count
	ReadSGOHeader(big_endian, buffer);
	// read name
	std::vector< SGONodeName > namenode;
	if (DataNameCount > 0)
	{
		namenode.resize(DataNameCount);
		for (int i = 0; i < DataNameCount; i++)
		{
			int nodepos = DataNameOffset + (i * 0x8);
			int nameoffset;
			Read4BytesData(big_endian, seg, buffer, nodepos);
			memcpy(&nameoffset, &seg, 4U);
			namenode[i].name = ReadUnicode(buffer, nodepos + nameoffset);

			Read4BytesData(big_endian, seg, buffer, nodepos + 4);
			memcpy(&namenode[i].id, &seg, 4U);
		}
	}
	// read data
	std::vector< SGONode > datanode;
	datanode.resize(DataNodeCount);
	std::string utf8str;
	for (int i = 0; i < DataNodeCount; i++)
	{
		int nodepos = DataNodeOffset + (i * 0xC);
		tinyxml2::XMLElement* xmlNode;

		ReadSGONode(big_endian, buffer, nodepos, datanode, i, xmlNode, header, xmlHeader);
		xmlNode->SetAttribute("index", i);
		// write name
		for (size_t j = 0; j < namenode.size(); j++)
		{
			if (namenode[j].id == i)
			{
				utf8str = WideToUTF8(namenode[j].name);
				xmlNode->SetAttribute("name", utf8str.c_str());
				break;
			}
		}
	}
}

void SGO::Read4BytesData(bool big_endian, unsigned char *seg, std::vector<char> buffer, int position)
{
	if (big_endian)
		Read4Bytes(seg, buffer, position);
	else
		Read4BytesReversed(seg, buffer, position);
}

void SGO::ReadSGOHeader( bool big_endian, std::vector<char> buffer)
{
	unsigned char seg[4];
	// read data node count
	int position = 0x8;
	Read4BytesData(big_endian, seg, buffer, position);
	memcpy(&DataNodeCount, &seg, 4U);
	// read data node offset
	position = 0xC;
	Read4BytesData(big_endian, seg, buffer, position);
	memcpy(&DataNodeOffset, &seg, 4U);
	// read data name count
	position = 0x10;
	Read4BytesData(big_endian, seg, buffer, position);
	memcpy(&DataNameCount, &seg, 4U);
	// read data name offset
	position = 0x14;
	Read4BytesData(big_endian, seg, buffer, position);
	memcpy(&DataNameOffset, &seg, 4U);
	// read data unknown count
	position = 0x18;
	Read4BytesData(big_endian, seg, buffer, position);
	memcpy(&DataUnkCount, &seg, 4U);
	// read data unknown offset
	position = 0x1C;
	Read4BytesData(big_endian, seg, buffer, position);
	memcpy(&DataUnkOffset, &seg, 4U);
}

void SGO::ReadSGONode(bool big_endian, std::vector<char> buffer, int nodepos, std::vector<SGONode>& datanode, int i, tinyxml2::XMLElement*& xmlNode, tinyxml2::XMLElement* header, tinyxml2::XMLElement* xmlHeader)
{
	int type = 0;
	unsigned char seg[4];
	Read4BytesData(big_endian, seg, buffer, nodepos);
	memcpy(&type, &seg, 4U);
	datanode[i].type = type;
	// read value
	switch (type) {
	case 0:{
		//this is an extra data
		int ptrnum, ptroffset;
		Read4BytesData(big_endian, seg, buffer, nodepos + 4);
		memcpy(&ptrnum, &seg, 4U);
		Read4BytesData(big_endian, seg, buffer, nodepos + 8);
		memcpy(&ptroffset, &seg, 4U);

		std::vector< SGONode > datavnode;
		datavnode.resize(ptrnum);

		xmlNode = header->InsertNewChildElement("ptr");
		for (int i = 0; i < ptrnum; i++)
		{
			int vnodepos = nodepos + ptroffset + (i * 0xC);
			tinyxml2::XMLElement* xmlVNode;

			ReadSGONode(big_endian, buffer, vnodepos, datavnode, i, xmlVNode, xmlNode, xmlHeader);
			xmlVNode->SetAttribute("index", i);
		}

		break;
	}
	case 1:
		Read4BytesData(big_endian, seg, buffer, nodepos + 8);
		memcpy(&datanode[i].ivalue, &seg, 4U);

		xmlNode = header->InsertNewChildElement("int");
		xmlNode->SetText(datanode[i].ivalue);
		break;
	case 2:
		Read4BytesData(big_endian, seg, buffer, nodepos + 8);
		memcpy(&datanode[i].fvalue, &seg, 4U);

		xmlNode = header->InsertNewChildElement("float");
		xmlNode->SetText(datanode[i].fvalue);
		break;
	case 3:{
		//this is a string
		int strsize, stroffset;
		Read4BytesData(big_endian, seg, buffer, nodepos + 4);
		memcpy(&strsize, &seg, 4U);
		Read4BytesData(big_endian, seg, buffer, nodepos + 8);
		memcpy(&stroffset, &seg, 4U);

		if (strsize > 0)
			datanode[i].strvalue = ReadUnicode(buffer, nodepos + stroffset);
		else
			datanode[i].strvalue = L"";

		std::string utf8str = WideToUTF8(datanode[i].strvalue);
		xmlNode = header->InsertNewChildElement("string");
		xmlNode->SetText(utf8str.c_str());
		break;
	}
	case 4:{
		//this is an extra data file
		int filesize, fileoffset;
		Read4BytesData(big_endian, seg, buffer, nodepos + 4);
		memcpy(&filesize, &seg, 4U);
		Read4BytesData(big_endian, seg, buffer, nodepos + 8);
		memcpy(&fileoffset, &seg, 4U);

		xmlNode = header->InsertNewChildElement("extra");

		int curpos = nodepos + fileoffset;
		std::vector<char> newbuf;
		for (int i = 0; i < filesize; i++)
			newbuf.push_back(buffer[curpos + i]);

		CheckDataType(newbuf, xmlNode, xmlHeader, curpos + buffer.size());

		break;
	}
	default:
		break;
	}
}
