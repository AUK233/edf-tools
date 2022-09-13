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
		
		std::string outfile = WideToUTF8(path) + "_DATA.xml";
		xml.SaveFile(outfile.c_str());
		/*
		tinyxml2::XMLPrinter printer;
		xml.Accept(&printer);
		auto xmlString = std::string{ printer.CStr() };

		std::wcout << UTF8ToWide(xmlString) + L"\n";
		*/
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
		// this is an extra data file
		int filesize, fileoffset;
		Read4BytesData(big_endian, seg, buffer, nodepos + 4);
		memcpy(&filesize, &seg, 4U);
		Read4BytesData(big_endian, seg, buffer, nodepos + 8);
		memcpy(&fileoffset, &seg, 4U);

		xmlNode = header->InsertNewChildElement("extra");

		int curpos = nodepos + fileoffset;
		// check for duplicate data
		std::string namestr = "SGO_" + std::to_string(buffer.size()) + "_" + std::to_string(curpos);
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

			std::vector<char> newbuf;
			for (int i = 0; i < filesize; i++)
				newbuf.push_back(buffer[curpos + i]);

			CheckDataType(newbuf, xmlHeader, namestr);
		}
		xmlNode->SetText(namestr.c_str());

		break;
	}
	default:
		break;
	}
	// only debug
	xmlNode->SetAttribute("debugPos", nodepos);
}

void SGO::Write(std::wstring path, tinyxml2::XMLNode* header)
{
	std::wcout << "Will output SGO file.\n";

	tinyxml2::XMLElement* mainData = header->FirstChildElement("Main");

	std::vector< char > bytes;
	bytes = WriteData(mainData, header);

	//Final write.
	std::ofstream newFile(path + L".sgo", std::ios::binary | std::ios::out | std::ios::ate);

	for (int i = 0; i < bytes.size(); i++)
	{
		newFile << bytes[i];
	}

	newFile.close();
	std::wcout << L"Conversion completed: " + path + L".sgo\n";
}

std::vector< char > SGO::WriteData(tinyxml2::XMLElement* mainData, tinyxml2::XMLNode* header)
{
	std::vector< char > bytes;

	tinyxml2::XMLElement* entry;
	// prefetch data size
	int nodePtrNum = 0;
	std::string dataName;
	for (entry = mainData->FirstChildElement(); entry != 0; entry = entry->NextSiblingElement())
	{
		DataNodeCount++;
		// get name count
		if (entry->Attribute("name"))
		{
			DataNameCount++;
			// preread string
			std::string namestr = entry->Attribute("name");
			bool subexist = false;
			// check for duplicates
			for (size_t i = 0; i < NodeString.size(); i++)
			{
				if (NodeString[i] == namestr)
				{
					subexist = true;
					break;
				}
			}

			if (!subexist)
				NodeString.push_back(namestr);
		}
		// if node is ptr or extra data
		GetNodeExtraData(entry, nodePtrNum);
	}
	// get sub data size
	for (entry = header->FirstChildElement("Subdata"); entry != 0; entry = entry->NextSiblingElement("Subdata"))
	{
		dataName = entry->Attribute("name");
		for (size_t i = 0; i < SubDataGroup.size(); i++)
		{
			if (SubDataGroup[i] == dataName)
			{
				ExtraData.push_back(GetExtraData(entry, dataName, header));
				break;
			}
		}
	}

	// compute node size
	int i_NdataCount = DataNodeCount * 0xC;
	int i_NptrCount = nodePtrNum * 0xC;
	int i_NnameCount = DataNameCount * 0x8;
	int i_NdataSize = 0x20 + i_NdataCount + i_NptrCount;
	// 16 byte alignment
	int a_datasize = i_NdataSize;
	if (i_NdataSize % 16 != 0)
		a_datasize = (i_NdataSize / 16 + 1) * 16;
	// calculate Total Size
	int i_NtotalSize = a_datasize + i_NnameCount;
	// 16 byte alignment
	int a_nodesize = i_NtotalSize;
	if (i_NtotalSize % 16 != 0)
		a_nodesize = (i_NtotalSize / 16 + 1) * 16;
	// add extra size
	int e_nodesize = a_nodesize;
	for (size_t i = 0; i < ExtraData.size(); i++)
	{
		ExtraDataPos.push_back(e_nodesize);
		e_nodesize += ExtraData[i].bytes.size();
	}
	WstrPos = e_nodesize;

	// out string
	std::sort(NodeString.begin(), NodeString.end());
	int strpos = 0;
	for (size_t i = 0; i < NodeString.size(); i++)
	{
		SGONodeName NN;
		NN.id = e_nodesize + strpos;
		strpos += (NodeString[i].length() * 2);
		strpos += 2;
		NN.name = UTF8ToWide(NodeString[i]);
		NodeWString.push_back(NN);
	}

	// get node data
	std::vector< char > NodeBytes(i_NdataCount);
	//NodeBytes.resize(i_NdataCount);
	std::vector< SGOExtraData > NodeData;
	std::vector< SGOExtraData > NodeName;
	int NodeIndex = 0;
	for (entry = mainData->FirstChildElement(); entry != 0; entry = entry->NextSiblingElement())
	{
		// write type
		NodeName.push_back( GetNodeName(entry, (a_datasize + (NodeName.size() * 8)), NodeIndex) );
		// write data
		NodeData.push_back( GetNodeData( entry, NodeBytes.size(), (0x20 + (NodeIndex * 0xC)), NodeBytes) );
		for (int i = 0; i < 12; i++)
			NodeBytes[NodeIndex * 0xC + i] = NodeData.back().bytes[i];
		NodeIndex++;
	}

	// push bytes!
	bytes.resize(0x20);
	bytes[0] = 0x53;
	bytes[1] = 0x47;
	bytes[2] = 0x4F;
	bytes[4] = 0x02;
	bytes[5] = 0x01;
	// node data
	memcpy(&bytes[0x8], &DataNodeCount, 4U);
	// always 0x20
	bytes[0xC] = 0x20;
	// node name
	memcpy(&bytes[0x10], &DataNodeCount, 4U);
	memcpy(&bytes[0x14], &a_datasize, 4U);
	// unknown
	memcpy(&bytes[0x1C], &i_NtotalSize, 4U);
	// write node data
	for (size_t i = 0; i < NodeBytes.size(); i++)
		bytes.push_back(NodeBytes[i]);
	// check alignment
	if (a_datasize > i_NdataSize)
	{
		for (int i = (a_datasize - i_NdataSize); i > 0; --i)
			bytes.push_back(0);
	}
	// write node type
	for (size_t i = 0; i < NodeName.size(); i++)
	{
		for (size_t j = 0; j < NodeName[i].bytes.size(); j++)
			bytes.push_back(NodeName[i].bytes[j]);
	}
	// check alignment
	if (a_nodesize > i_NtotalSize)
	{
		for (int i = (a_nodesize - i_NtotalSize); i > 0; --i)
			bytes.push_back(0);
	}
	// write extra file
	for (size_t i = 0; i < ExtraData.size(); i++)
	{
		for (size_t j = 0; j < ExtraData[i].bytes.size(); j++)
			bytes.push_back(ExtraData[i].bytes[j]);
	}
	// write wide string
	for (size_t i = 0; i < NodeWString.size(); i++)
		PushWStringToVector(NodeWString[i].name, &bytes);

	// debug only
	std::wcout << L"String Size: " + ToString(int(NodeString.size())) + L"\n\n";

	std::wcout << L"DataNodeCount: " + ToString(DataNodeCount) + L"\n";
	std::wcout << L"DataNameCount: " + ToString(DataNameCount) + L"\n";
	std::wcout << L"nodePtrNum: " + ToString(nodePtrNum) + L"\n";
	std::wcout << L"Total Data Size: " + ToString(i_NtotalSize) + L"\n";
	std::wcout << L"Align Data Size: " + ToString(a_nodesize) + L"\n";
	std::wcout << L"SubDataGroup num: " + ToString(int(SubDataGroup.size())) + L"\n";
	std::wcout << L"ExtraData num: " + ToString(int(ExtraData.size())) + L"\n";

	return bytes;
}

void SGO::GetNodeExtraData(tinyxml2::XMLElement* entry, int& nodePtrNum)
{
	std::string nodeName = entry->Name();
	if (nodeName == "ptr")
	{
		tinyxml2::XMLElement* entry2;
		for (entry2 = entry->FirstChildElement(); entry2 != 0; entry2 = entry2->NextSiblingElement())
		{
			nodePtrNum++;
			GetNodeExtraData(entry2, nodePtrNum);
		}
	}
	else if (nodeName == "string")
	{
		// preread string
		if (entry->GetText())
		{
			std::string namestr = entry->GetText();
			bool subexist = false;
			// check for duplicates
			for (size_t i = 0; i < NodeString.size(); i++)
			{
				if (NodeString[i] == namestr)
				{
					subexist = true;
					break;
				}
			}

			if (!subexist)
				NodeString.push_back(namestr);
		}
	}
	else if (nodeName == "extra")
	{
		std::string namestr = entry->GetText();
		bool subexist = false;
		// check for duplicates
		for (size_t i = 0; i < SubDataGroup.size(); i++)
		{
			if (SubDataGroup[i] == namestr)
			{
				subexist = true;
				break;
			}
		}

		if (!subexist)
			SubDataGroup.push_back(namestr);
	}
}

SGOExtraData SGO::GetExtraData(tinyxml2::XMLElement* entry, std::string dataName, tinyxml2::XMLNode* header)
{
	SGOExtraData out;
	out.name = dataName;
	out.bytes = CheckDataType(entry, header);
	return out;
}

SGOExtraData SGO::GetNodeData(tinyxml2::XMLElement* entry, int size, int pos, std::vector< char > & NodeBytes)
{
	SGOExtraData out;
	out.bytes.resize(12U);

	std::string nodeType = entry->Name();
	if (nodeType == "ptr")
	{
		out.bytes[0] = 0;
		int offset = 0x20 + size;
		tinyxml2::XMLElement* entry2;
		// first, set placeholders
		int count = 0;
		for (entry2 = entry->FirstChildElement(); entry2 != 0; entry2 = entry2->NextSiblingElement())
			count++;
		for (int i = count * 0xC; i > 0; --i)
			NodeBytes.push_back(0);
		// then, read data
		std::vector< SGOExtraData > Data;
		count = 0;
		for (entry2 = entry->FirstChildElement(); entry2 != 0; entry2 = entry2->NextSiblingElement())
		{
			Data.push_back(GetNodeData(entry2, NodeBytes.size(), (offset + (count * 0xC)), NodeBytes));
			int wpos = (count * 0xC) + size;
			for (int i = 0; i < 12; i++)
				NodeBytes[wpos + i] = Data.back().bytes[i];
			count++;
		}
		// last, write data
		int value[2];
		value[0] = count;
		value[1] = size - pos;
		memcpy(&out.bytes[4], &value, 8U);
	}
	else if (nodeType == "int")
	{
		out.bytes[0] = 1;
		out.bytes[4] = 4;
		int value = entry->IntText();
		memcpy(&out.bytes[8], &value, 4U);
	}
	else if (nodeType == "float")
	{
		out.bytes[0] = 2;
		out.bytes[4] = 4;
		float value = entry->FloatText();
		memcpy(&out.bytes[8], &value, 4U);
	}
	else if (nodeType == "string")
	{
		out.bytes[0] = 3;
		if (entry->GetText())
		{
			out.name = entry->GetText();
			std::wstring wstr = UTF8ToWide(out.name);
			for (size_t i = 0; i < NodeWString.size(); i++)
			{
				if (NodeWString[i].name == wstr)
				{
					int value[2];
					value[0] = NodeWString[i].name.size();
					value[1] = NodeWString[i].id - pos;
					memcpy(&out.bytes[4], &value, 8U);

					break;
				}
			}
		}
		else
		{
			memcpy(&out.bytes[8], &WstrPos, 4U);
		}
	}
	else if (nodeType == "extra")
	{
		out.bytes[0] = 4;
		out.name = entry->GetText();

		for (size_t i = 0; i < ExtraData.size(); i++)
		{
			if (ExtraData[i].name == out.name)
			{
				int value[2];
				value[0] = ExtraData[i].bytes.size();
				value[1] = ExtraDataPos[i] - pos;
				memcpy(&out.bytes[4], &value, 8U);

				break;
			}
		}
	}

	return out;
}

SGOExtraData SGO::GetNodeName(tinyxml2::XMLElement* entry, int pos, int NodeIndex)
{
	SGOExtraData out;
	out.bytes.resize(8U);

	if (entry->Attribute("name"))
	{
		out.name = entry->Attribute("name");
		std::wstring wstr = UTF8ToWide(out.name);
		for (size_t i = 0; i < NodeWString.size(); i++)
		{
			if (NodeWString[i].name == wstr)
			{
				int value[2];
				value[0] = NodeWString[i].id - pos;
				value[1] = NodeIndex;
				memcpy(&out.bytes[0], &value, 8U);

				break;
			}
		}
	}

	return out;
}
