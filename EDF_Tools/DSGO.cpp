#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include <algorithm>

#include <iostream>
#include <locale>
#include <locale.h>
#include "util.h"
#include "include/tinyxml2.h"
#include "DSGO.h"
#include "SGO.h"
#include "Middleware.h"

void DSGO::ReadData(const std::vector<char>& buffer, tinyxml2::XMLElement* header, tinyxml2::XMLElement* xmlHeader)
{
	// check header
	int big_endian = 0;
	int headerType = ReadInt32(&buffer[0], 1);
	if (headerType == 1330074436) {
		big_endian == 1;
	} else {
		headerType = ReadInt32(&buffer[0], 0);
		if (headerType != 1330074436) {
			std::wcout << L"Warning: the input file is not DSGO!\n";
			return;
		}
	}
	// ok, We don't know what it does.
	int unk0x4 = ReadInt32(&buffer[4], big_endian);

	// get node info
	DataNodeCount = ReadInt32(&buffer[0x8], big_endian);
	DataNodeOffset = ReadInt32(&buffer[0xC], big_endian);
	// First, read all nodes
	std::vector< DSGOStandardNode > datanode;
	datanode.resize(DataNodeCount);
	for (int i = 0; i < DataNodeCount; i++)
	{
		int nodepos = DataNodeOffset + (i * 0x10);
		datanode[i].v.vi = ReadInt64(&buffer[nodepos], big_endian);
		datanode[i].type = ReadInt64(&buffer[nodepos + 8], big_endian);
		datanode[i].pos = nodepos;
	}
	// Next, output the first node, which should be of type 3
	ReadDSGONode(big_endian, buffer, DataNodeOffset, datanode, 0, header, xmlHeader);
}

void DSGO::ReadDSGONode(bool big_endian, const std::vector<char>& buffer, int nodepos, const std::vector<DSGOStandardNode>& datanode, int SN, tinyxml2::XMLElement* header, tinyxml2::XMLElement* xmlHeader)
{
	uintptr_t nodePos = nodepos + datanode[SN].v.vi;

	int nameTableOfs = ReadInt32(&buffer[nodePos], big_endian);
	int nameSize = ReadInt32(&buffer[nodePos + 4], big_endian);
	int varTableOfs = ReadInt32(&buffer[nodePos + 8], big_endian);
	int varSize = ReadInt32(&buffer[nodePos + 12], big_endian);

	// Read node name list
	std::vector< DSGONameTalbe > names(nameSize);
	uintptr_t nameOfs = nodePos + nameTableOfs;
	for (int i = 0; i < nameSize; i++)
	{
		int ofs = ReadInt32(&buffer[nameOfs], big_endian);
		names[i].wstr = ReadUnicode(buffer, nameOfs + ofs, big_endian);
		names[i].index = ReadInt32(&buffer[nameOfs + 4], big_endian);
		nameOfs += 8;
	}
	// Read node list
	std::vector< int32_t > nodes(varSize);
	uintptr_t nodeOfs = nodePos + varTableOfs;
	for (int i = 0; i < varSize; i++) {
		nodes[i] = ReadInt32(&buffer[nodeOfs], big_endian);
		nodeOfs += 4;
	}

	// Output node
	for (int i = 0; i < varSize; i++) {
		int64_t type = datanode[nodes[i]].type;
		tinyxml2::XMLElement* xmlNode;
		switch (type) {
		case 0: {
			xmlNode = header->InsertNewChildElement("val");
			xmlNode->SetText(datanode[nodes[i]].v.vf);
			ReadDSGONodeSetNodeName(names, i, xmlNode);
			break;
		}
		case 1: {
			//this is a string
			int64_t stroffset = datanode[nodes[i]].v.vi;

			std::string utf8str = "";
			if (stroffset > 0) {
				utf8str = WideToUTF8(ReadUnicode(buffer, datanode[nodes[i]].pos + stroffset, big_endian));
			}

			xmlNode = header->InsertNewChildElement("str");
			xmlNode->SetText(utf8str.c_str());
			ReadDSGONodeSetNodeName(names, i, xmlNode);
			break;
		}
		case 2: {
			//this is an extra data file
			int64_t dataPos = datanode[nodes[i]].pos + datanode[nodes[i]].v.vi;
			int filesize = ReadInt32(&buffer[dataPos], big_endian);
			int fileoffset = ReadInt32(&buffer[dataPos +4], big_endian);
			// Maybe 16-byte alignment is needed

			xmlNode = header->InsertNewChildElement("extra");
			int curpos = dataPos + fileoffset;
			// check for duplicate data
			std::string namestr = "DSGO_" + std::to_string(buffer.size()) + "_" + std::to_string(curpos);
			bool subexist = false;
			for (size_t i = 0; i < SubDataGroup.size(); i++)
			{
				if (SubDataGroup[i] == namestr)
				{
					subexist = true;
					break;
				}
			}
			if (!subexist)
			{
				SubDataGroup.push_back(namestr);

				std::vector<char> newbuf(buffer.begin() + curpos, buffer.begin() + curpos + filesize);

				tinyxml2::XMLElement* NewXml = xmlHeader->InsertNewChildElement("Subdata");
				NewXml->SetAttribute("name", namestr.c_str());
				NewXml->SetAttribute("header", "RAW");
				std::string rawstr = ReadRaw(newbuf, 0, newbuf.size());
				NewXml->SetText(rawstr.c_str());
			}
			xmlNode->SetText(namestr.c_str());

			ReadDSGONodeSetNodeName(names, i, xmlNode);
			break;
		}
		case 3: {
			//this is a structured data
			xmlNode = header->InsertNewChildElement("ptr");
			if (datanode[nodes[i]].v.vi) {
				ReadDSGONode(big_endian, buffer, datanode[nodes[i]].pos, datanode, nodes[i], xmlNode, xmlHeader);
			}
			ReadDSGONodeSetNodeName(names, i, xmlNode);
			break;
		}
		case 4: {
			//Type4 that need to be parsed
			xmlNode = header->InsertNewChildElement("type4");
			xmlNode->SetText(datanode[nodes[i]].pos);
			//ReadDSGONodeSetNodeName(names, i, xmlNode);
			break;
		}
		default:
			break;
		}
	}

	/*
	switch (type) {
	case 0: {
		
	}
	}*/
}

void DSGO::ReadDSGONodeSetNodeName(const std::vector<DSGONameTalbe>& nameList, int SN, tinyxml2::XMLElement*& xmlNode)
{
	xmlNode->SetAttribute("index", SN);
	for (size_t i = 0; i < nameList.size(); i++) {
		if (nameList[i].index == SN) {
			std::string utf8str = WideToUTF8(nameList[i].wstr);
			xmlNode->SetAttribute("name", utf8str.c_str());
			break;
		}
	}
}
