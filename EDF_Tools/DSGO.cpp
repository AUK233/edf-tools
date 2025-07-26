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

//#define DEBUGMODE

DSGOType4Mapping_t Type4NodePreset[] = {
	{5, "add"},
	{6, "sub"},
	{7, "mul"},
	{8, "div"},
	{9, "mod"}, // like https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/fmod-fmodf?view=msvc-170
	{10, "shl"},
	{11, "shr"},
	{12, "and"},
	{13, "or"}
};

DSGOType4Mapping_t Type4NodePreset4[] = {
	{0x80000000, "fpow"}, // pow(v1, v2), -2147483648
	{0x80000001, "flog2"},
	{0x80000002, "fget_L"}, // if +0>+28h, +0 is +28h
	{0x80000003, "fget_B"}, // if +28h>+0, +28h is +8
	{0x80000004, "fmax3"}, // take the biggest of the last 3 values
	{0x80000005, "fgetM1"}, // limit the last value to 0.0 - 1.0
	{0x80000006, "fsma"}, // v2 - v1 * v3 + v1
	{0x80000007, "fsin"}, // sin(v1)
	{0x80000008, "fcos"}, // cos(v1)
	{0x80000009, "ftan"}, // tan(v1)
	{0x8000000A, "fasin"}, // asin(v1)
	{0x8000000B, "facos"}, // acos(v1)
	{0x8000000C, "fatan"} // atan(v1)
};


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
		datanode[i].readCount = 0;
		datanode[i].readByType4 = 0;
		datanode[i].hasMark = 0;
		datanode[i].markDone = 0;
	}
	// Next, find the type4 loaded node
	PreReadDSGONode(big_endian, buffer, datanode);
	// Last, output the first node, which should be of type 3
	ReadDSGONode(big_endian, buffer, DataNodeOffset, datanode, 0, header, xmlHeader);
}

void DSGO::PreReadDSGONode(bool big_endian, const std::vector<char>& buffer, std::vector<DSGOStandardNode>& datanode)
{
	for (int i = 0; i < DataNodeCount; i++) {
		// preprocessing type4
		if (datanode[i].type == 4) {
			if (datanode[i].v.vi) {
				datanode[i].v.vi += datanode[i].pos;
				// Size! Not the node count!
				int dataSize = ReadInt32(&buffer[datanode[i].v.vi], big_endian);
				int dataOffset = ReadInt32(&buffer[datanode[i].v.vi + 4], big_endian);
				int dataPos = datanode[i].v.vi + dataOffset;

				int startPos = 0;
				int type;
				while (startPos < dataSize) {
					int curPos = dataPos + startPos;
					type = ReadInt32(&buffer[curPos], big_endian);
					startPos += 4;
					if (type == 1) {
						startPos += 8;
					}
					else if (type == 2) {
						std::wcout << L"Type 2 of type4 is detected in " + ToString(curPos) + L"\n";
						system("pause");
					}
					else if (type == 3) {
						// Important, here we will assign identifiers to the nodes.
						int nodeIndex = ReadInt32(&buffer[curPos+4], big_endian);
						if (nodeIndex < DataNodeCount) {
							datanode[nodeIndex].readByType4 = 1;
							datanode[nodeIndex].hasMark = 1;
							std::string tempStr = "mark";
							tempStr += std::to_string(nodeIndex);
							datanode[nodeIndex].nodeMark = tempStr;
						}
						startPos += 4;
					}
					else if (type == 4) {
						startPos += 4;
					}
					// end
				}
			}
		}
		// end
	}
}

void DSGO::ReadDSGONode(bool big_endian, const std::vector<char>& buffer, int nodepos, std::vector<DSGOStandardNode>& datanode, int SN, tinyxml2::XMLElement* header, tinyxml2::XMLElement* xmlHeader)
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
		tinyxml2::XMLElement* xmlNode;

		// first, check for duplicates
		if (datanode[nodes[i]].readCount) {
			xmlNode = header->InsertNewChildElement("reuse");
			// node name needs to be preserved
			ReadDSGONodeSetNodeName(names, i, xmlNode);

			std::string nodeMark;
			if (datanode[nodes[i]].hasMark) {
				nodeMark = datanode[nodes[i]].nodeMark;
			} else {
				nodeMark = "mark" + std::to_string(nodes[i]);
				datanode[nodes[i]].nodeMark = nodeMark;
				datanode[nodes[i]].hasMark = 1;
			}
			xmlNode->SetAttribute("MARKNAME", nodeMark.c_str());

			if (!datanode[nodes[i]].markDone) {
				datanode[nodes[i]].xmlNode->SetAttribute("MARKNAME", nodeMark.c_str());
				datanode[nodes[i]].markDone = 1;
			}

			//xmlNode->SetAttribute("reusable", datanode[nodes[i]].readCount);
			datanode[nodes[i]].readCount = 999;
			// skip next operation
			continue;
		}
		// node read count + 1
		datanode[nodes[i]].readCount += 1;

		int64_t type = datanode[nodes[i]].type;
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
			// this is an extra data file
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
				
				/*tinyxml2::XMLElement* NewXml = xmlHeader->InsertNewChildElement("Subdata");
				NewXml->SetAttribute("name", namestr.c_str());
				NewXml->SetAttribute("header", "RAW");
				std::string rawstr = ReadRaw(newbuf, 0, newbuf.size());
				NewXml->SetText(rawstr.c_str());*/
				
				CheckDataType(newbuf, xmlHeader, namestr);
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
			xmlNode = header->InsertNewChildElement("math");
			int64_t dataOfs = datanode[nodes[i]].v.vi;
			if (dataOfs) {
				ReadDSGONodeType4(big_endian, buffer, dataOfs, datanode, nodes[i], xmlNode);
			}
			ReadDSGONodeSetNodeName(names, i, xmlNode);
			break;
		}
		default:
			xmlNode = header->InsertNewChildElement("error");
			break;
		}

#if defined(DEBUGMODE)
		xmlNode->SetAttribute("testSN", nodes[i]);
		xmlNode->SetAttribute("pos", datanode[nodes[i]].pos);
#endif

		// save xml node
		datanode[nodes[i]].xmlNode = xmlNode;
		// check loaded by type4
		if (datanode[nodes[i]].readByType4) {
			if (!datanode[nodes[i]].markDone) {
				xmlNode->SetAttribute("MARKNAME", datanode[nodes[i]].nodeMark.c_str());
				datanode[nodes[i]].markDone = 1;
			}
		}
		// end
	}
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

void DSGO::ReadDSGONodeType4(bool big_endian, const std::vector<char>& buffer, int nodepos, const std::vector<DSGOStandardNode>& datanode, int SN, tinyxml2::XMLElement*& xmlNode)
{
	// Size! Not the node count!
	int dataSize = ReadInt32(&buffer[nodepos], big_endian);
	int dataOffset = ReadInt32(&buffer[nodepos + 4], big_endian);
	int dataPos = nodepos + dataOffset;
	//xmlNode->SetAttribute("pos", dataPos);

	int startPos = 0;
	int type;
	tinyxml2::XMLElement* xmlVNode;
	while (startPos < dataSize) {
		int curPos = dataPos + startPos;
		type = ReadInt32(&buffer[curPos], big_endian);
		switch (type)
		{
		case 1: {
			// it is FP64
			xmlVNode = xmlNode->InsertNewChildElement("value");
			xmlVNode->SetText(ReadFP64(&buffer[curPos + 4], big_endian));
			startPos += 12;
			break;
		}
		case 2: {
			// type 2 is wstring, but we don't know structure
			xmlVNode = xmlNode->InsertNewChildElement("type4_2");
			xmlVNode->SetAttribute("pos", curPos);
			startPos += 4;
			break;
		}
		case 3: {
			xmlVNode = xmlNode->InsertNewChildElement("load");
			int nodeIndex = ReadInt32(&buffer[curPos + 4], big_endian);
			xmlVNode->SetText(datanode[nodeIndex].nodeMark.c_str());
			startPos += 8;
			break;
		}
		case 4: {
			// 4 is complex arithmetic and needs more information
			int to4type = ReadInt32(&buffer[curPos + 4], big_endian);
			
			char* opcode_name = 0;
			for (int i = 0; i < _countof(Type4NodePreset4); i++) {
				if (to4type == Type4NodePreset4[i].number) {
					opcode_name = Type4NodePreset4[i].name;
					goto checkOpcode4;
				}
			}

			checkOpcode4:
			if (!opcode_name) {
				std::wcout << L"Warning: Unknown type4 opcode " + ToString(to4type) + L" at " + ToString(curPos) + L"\n";
				return;
			}
			xmlVNode = xmlNode->InsertNewChildElement("opcode");
			xmlVNode->SetText(opcode_name);

			startPos += 8;
			break;
		}
		default:
			if (!type) {
				return;
			}

			char* opcode_name = 0;
			for (int i = 0; i < _countof(Type4NodePreset); i++) {
				if(type == Type4NodePreset[i].number) {
					opcode_name = Type4NodePreset[i].name;
					goto checkOpcode;
				}
			}

			checkOpcode:
			if (!opcode_name) {
				return;
			}
			xmlVNode = xmlNode->InsertNewChildElement("opcode");
			xmlVNode->SetText(opcode_name);

			startPos += 4;
			break;
		}
	}
}

// ==================================================================================================

void DSGO::Write(const std::wstring& path, tinyxml2::XMLNode* header)
{
	std::wcout << "Will output DSGO file.\n";

	tinyxml2::XMLElement* mainData = header->FirstChildElement("Main");

	std::vector< char > bytes;
	bytes = WriteData(mainData, header);

	//Final write.
	std::ofstream newFile(path + L".sgo", std::ios::binary | std::ios::out | std::ios::ate);

	newFile.write(bytes.data(), bytes.size());

	newFile.close();
	std::wcout << L"Conversion completed: " + path + L".sgo\n";
}

std::vector< char > DSGO::WriteData(tinyxml2::XMLElement* mainData, tinyxml2::XMLNode* header)
{
	// initialization
	outHeader.header = 1330074436; // DSGO
	outHeader.unk0x4 = 0x10;
	outHeader.nodeCount = 1; // main node need to be counted.
	outHeader.nodeOffset = 0x10;

	std::vector< char > bytes;

	tinyxml2::XMLElement* entry = mainData->FirstChildElement();
	// if empty content, return empty sgo
	if (!entry)
	{
		bytes.resize(0x20);
		inNode_t mainNode = {0,3};
		memcpy(&bytes[0], &outHeader, 16);
		memcpy(&bytes[0x10], &mainNode, 16);

		return bytes;
	}

	// record type4 opcode map
	map_type4opcode.rehash(_countof(Type4NodePreset) + _countof(Type4NodePreset4) + 2);
	for (int i = 0; i < _countof(Type4NodePreset); i++)
	{
		map_type4opcode[Type4NodePreset[i].name] = Type4NodePreset[i].number;
	}
	for (int i = 0; i < _countof(Type4NodePreset4); i++)
	{
		map_type4opcode[Type4NodePreset4[i].name] = Type4NodePreset4[i].number;
	}

	// get sub data size
	for (entry = header->FirstChildElement("Subdata"); entry != 0; entry = entry->NextSiblingElement("Subdata"))
	{
		WriteData_SetExtraData(entry, header);
	}

	// get main node
	WriteData_GetPtrData(mainData);

	// get node count
	int i_nodeCount = v_outNode.size();
	outHeader.nodeCount = i_nodeCount;
	// note: size includes header
	int i_nodeSize = 0x10 + (i_nodeCount * 16);

	std::vector<char> v_table;
	for(int i = 0; i < v_outPtrTable.size(); i++)
	{
		std::vector<char> v_temp = v_outPtrTable[i]->WriteData(this, i_nodeSize + v_table.size());
		v_table.insert(v_table.end(), v_temp.begin(), v_temp.end());

		delete v_outPtrTable[i];
		v_temp.clear();
	}
	v_outPtrTable.clear();
	v_outExtraData.clear();


	// write header
	bytes.resize(i_nodeSize);
	memcpy(&bytes[0], &outHeader, 16);
	int nodePos = 0x10;
	for (int i = 0; i < v_outNode.size(); i++)
	{
		memcpy(&bytes[nodePos], &v_outNode[i], 16);
		nodePos += 16;
	}

	// write data table
	bytes.insert(bytes.end(), v_table.begin(), v_table.end());
	v_table.clear();

	// update string offset
	int bufferSize = bytes.size();
	int str_curpos, str_curofs;
	for (int i = 0; i < v_update_string.size(); i++) {
		str_curpos = v_update_string[i].pos;
		str_curofs = v_update_string[i].offset + bufferSize;
		memcpy(&bytes[str_curpos], &str_curofs, 4);
	}
	v_update_string.clear();

	// write string
	bytes.insert(bytes.end(), v_wstring.begin(), v_wstring.end());
	v_wstring.clear();

	return bytes;
}

int DSGO::WriteData_WideString(std::string str)
{
	// check map
	auto it = map_string.find(str);
	if (it != map_string.end()) {
		return it->second;
	}
	// write string
	int offset = v_wstring.size();
	map_string[str] = offset;

	// convert to wide string
	std::wstring wstr = UTF8ToWide(str);
	auto strnBytes = reinterpret_cast<const char*>(&wstr[0]);
	int size = wstr.size() * 2;

	for (int i = 0; i < size; i += 2)
	{
		v_wstring.push_back(strnBytes[i]);
		v_wstring.push_back(strnBytes[i + 1]);
	}
	// Zero terminate
	v_wstring.push_back(0);
	v_wstring.push_back(0);

	return offset;
}

void DSGO::WriteData_SetNodeIndex(const char* pName, int index)
{
	if (!pName) {
		return;
	}

	std::string str = pName;
	if (str.empty()) {
		return;
	}

	map_mark[str] = index;
}

void DSGO::WriteData_SetExtraData(tinyxml2::XMLElement* entry, tinyxml2::XMLNode* header)
{
	outExtraData_t out;
	out.name = entry->Attribute("name");
	out.bytes = CheckDataType(entry, header);

	size_t dataSize = out.bytes.size();
	out.size = dataSize;
	size_t alignSize = dataSize % 16;
	if (alignSize) {
		for (size_t i = alignSize; i < 16; i++) {
			out.bytes.push_back(0);
		}
	}

	map_outExtraData[out.name] = v_outExtraData.size();

	v_outExtraData.push_back(out);
}

int DSGO::WriteData_GetNodeIndex(const char* pName)
{
	if (!pName) {
		return 0;
	}

	std::string str = pName;
	if (str.empty()) {
		return 0;
	}

	auto it = map_mark.find(str);
	if (it != map_mark.end()) {
		return it->second;
	}

	return 0;
}

int DSGO::WriteData_GetValueData(tinyxml2::XMLElement* xmlNode)
{
	int node_index = v_outNode.size();
	WriteData_SetNodeIndex(xmlNode->Attribute("MARKNAME"), node_index);

	inNode_t node;
	node.type = 0;
	node.v.vf = xmlNode->DoubleText();

	v_outNode.push_back(node);

	return node_index;
}

int DSGO::WriteData_GetStringData(tinyxml2::XMLElement* xmlNode)
{
	int node_index = v_outNode.size();
	WriteData_SetNodeIndex(xmlNode->Attribute("MARKNAME"), node_index);

	// set null node
	inNode_t node;
	node.v.vi = 0;
	node.type = 1;
	v_outNode.push_back(node);

	// get string
	std::string str = xmlNode->GetText();
	int offset = WriteData_WideString(str);

	updateDataOffset_t out_dataup;
	out_dataup.pos = 0x10 + (node_index * 16);
	out_dataup.offset = offset - out_dataup.pos;
	v_update_string.push_back(out_dataup);

	return node_index;
}

int DSGO::WriteData_GetExtraData(tinyxml2::XMLElement* xmlNode)
{
	inNode_t ptrNode = { 0, 2 };
	int node_index = v_outNode.size();

	WriteData_SetNodeIndex(xmlNode->Attribute("MARKNAME"), node_index);
	v_outNode.push_back(ptrNode);

	std::string str = xmlNode->GetText();
	int data_index = WriteData_GetExtraDataIndex(str);
	if (data_index == -1) {
		return node_index;
	}

	// 0 is data size, 1 is data offset, other is alignment
	int i_data[4];
	i_data[0] = v_outExtraData[data_index].size;
	i_data[1] = 0x10;
	i_data[2] = 0;
	i_data[3] = 0;

	// set buffer
	std::vector<char> buffer(0x10);
	memcpy(&buffer[0], &i_data[0], 16);
	buffer.insert(buffer.end(), v_outExtraData[data_index].bytes.begin(), v_outExtraData[data_index].bytes.end());

	// add to table
	DSGOoutExtraData_t* ptrTable = new DSGOoutExtraData_t;
	v_outPtrTable.push_back(ptrTable);
	ptrTable->nodeIndex = node_index;
	ptrTable->buffer = buffer;

	return node_index;
}

int DSGO::WriteData_GetExtraDataIndex(std::string str)
{
	auto it = map_outExtraData.find(str);
	if (it != map_outExtraData.end()) {
		return it->second;
	}

	return -1;
}

int DSGO::WriteData_GetPtrData(tinyxml2::XMLElement* xmlNode)
{
	inNode_t ptrNode = { 0,3 };
	int node_index = v_outNode.size();

	WriteData_SetNodeIndex(xmlNode->Attribute("MARKNAME"), node_index);
	v_outNode.push_back(ptrNode);

	DSGOoutPtrTable_t* ptrTable = new DSGOoutPtrTable_t;
	v_outPtrTable.push_back(ptrTable);
	// initialize pointer
	ptrTable->table = { 0, 0, 0, 0 };
	ptrTable->nodeIndex = node_index;

	tinyxml2::XMLElement* entry;
	for (entry = xmlNode->FirstChildElement(); entry != 0; entry = entry->NextSiblingElement())
	{
		int nodeIndex;
		std::string nodeType = entry->Name();
		if (nodeType == "val") {
			nodeIndex = WriteData_GetValueData(entry);
		}
		else if (nodeType == "str")
		{
			nodeIndex = WriteData_GetStringData(entry);
		}
		else if (nodeType == "extra")
		{
			nodeIndex = WriteData_GetExtraData(entry);
		}
		else if (nodeType == "ptr")
		{
			nodeIndex = WriteData_GetPtrData(entry);
		}
		else if (nodeType == "math")
		{
			nodeIndex = WriteData_GetMathData(entry);
		}
		else if (nodeType == "reuse")
		{
			nodeIndex = WriteData_GetReuseData(entry);
		}
		else {
			continue;
		}

		ptrTable->node.push_back(nodeIndex);

		outNameTable_t nameTable;

		auto pName = entry->Attribute("name");
		if (pName) {
			nameTable.name = pName;
			nameTable.varIndex = ptrTable->node.size() - 1;
			nameTable.offset = WriteData_WideString(nameTable.name);

			ptrTable->nameTable.push_back(nameTable);
		}
	}

	int nodeCount = ptrTable->node.size();
	if (!nodeCount) {
		return node_index;
	}

	ptrTable->table.varTableOffset = 0x10;
	ptrTable->table.varTableCount = nodeCount;

	int nameCount = ptrTable->nameTable.size();
	if (nameCount) {
		// Looks like it needs to be sorted
		std::sort(ptrTable->nameTable.begin(), ptrTable->nameTable.end());
	}
	ptrTable->table.nameTableOffset = 0x10 + (nodeCount * 4);
	ptrTable->table.nameTableCount = nameCount;

	return node_index;
}

int DSGO::WriteData_GetMathData(tinyxml2::XMLElement* xmlNode)
{
	inNode_t ptrNode = { 0, 4 };
	int node_index = v_outNode.size();

	WriteData_SetNodeIndex(xmlNode->Attribute("MARKNAME"), node_index);
	v_outNode.push_back(ptrNode);

	// 0 is data size, 1 is data offset
	int i_data[2];
	i_data[1] = 8;

	// set buffer to be 16-byte aligned.
	char maxBuffer[16];
	// set data buffer
	int i_BufferSize;
	std::vector<char> buffer;

	// get node
	for (tinyxml2::XMLElement* entry = xmlNode->FirstChildElement(); entry != 0; entry = entry->NextSiblingElement()) {
		std::string nodeType = entry->Name();
		if (nodeType == "value") {
			*(int*)&maxBuffer[0] = 1;
			*(double*)&maxBuffer[4] = entry->DoubleText();
			i_BufferSize = 12;
		}
		else if (nodeType == "load")
		{
			*(int*)&maxBuffer[0] = 3;
			*(int*)&maxBuffer[4] = WriteData_GetNodeIndex(entry->GetText());
			i_BufferSize = 8;
		}
		else if (nodeType == "opcode")
		{
			std::string opcode = entry->GetText();

			auto it = map_type4opcode.find(opcode);
			if (it == map_type4opcode.end()) {
				continue;
			}

			int i_opcode = it->second;
			if (i_opcode < 0) {
				*(int*)&maxBuffer[0] = 4;
				*(int*)&maxBuffer[4] = i_opcode;
				i_BufferSize = 8;
			}
			else {
				*(int*)&maxBuffer[0] = i_opcode;
				i_BufferSize = 4;
			}
		}
		else {
			continue;
		}

		buffer.insert(buffer.end(), maxBuffer, maxBuffer + i_BufferSize);
	}

	// set buffer to 0
	ZeroMemory(maxBuffer, 16);
	buffer.insert(buffer.end(), maxBuffer, maxBuffer + 4);
	i_data[0] = buffer.size();

	// add to table
	DSGOoutExtraData_t* ptrTable = new DSGOoutExtraData_t;
	v_outPtrTable.push_back(ptrTable);
	ptrTable->nodeIndex = node_index;

	ptrTable->buffer.resize(8);
	memcpy(&ptrTable->buffer[0], &i_data[0], 8);
	ptrTable->buffer.insert(ptrTable->buffer.end(), buffer.begin(), buffer.end());
	// check alignment
	int align = ptrTable->buffer.size() % 16;
	if(align) {
		ptrTable->buffer.insert(ptrTable->buffer.end(), maxBuffer, maxBuffer + (16 - align));
	}

	return node_index;
}

int DSGO::WriteData_GetReuseData(tinyxml2::XMLElement* xmlNode)
{
	return WriteData_GetNodeIndex(xmlNode->Attribute("MARKNAME"));
}

std::vector<char> DSGOoutPtrTable_t::WriteData(DSGO* p, int baseOffset)
{
	// write offset to node
	p->v_outNode[nodeIndex].v.vi = baseOffset - (nodeIndex * 16) - 0x10;

	// write table data
	int i_BaseSize = 0x10 + (table.varTableCount * 4);
	int i_NameTableSize = table.nameTableCount * 8;

	std::vector<char> buffer(i_BaseSize + i_NameTableSize);
	memcpy(&buffer[0], &table, 16);
	memcpy(&buffer[0x10], node.data(), (i_BaseSize - 0x10));

	// 0 is string offset, 1 is index
	int i_temp[2];
	i_temp[0] = -2; // set to debug value
	int nameCurPos = i_BaseSize;
	for (int i = 0; i < table.nameTableCount; i++)
	{
		i_temp[1] = nameTable[i].varIndex;
		memcpy(&buffer[nameCurPos], &i_temp, 8);

		// write to update string
		DSGO::updateDataOffset_t out_dataup;
		out_dataup.pos = baseOffset + nameCurPos;
		out_dataup.offset = nameTable[i].offset - out_dataup.pos;
		p->v_update_string.push_back(out_dataup);

		nameCurPos += 8;
	}

	return buffer;
}

std::vector<char> DSGOoutExtraData_t::WriteData(DSGO* p, int baseOffset)
{
	// write offset to node
	p->v_outNode[nodeIndex].v.vi = baseOffset - (nodeIndex * 16) - 0x10;

	return buffer;
}
