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
		datanode[i].readCount = 0;
		datanode[i].readByType4 = 0;
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
			int64_t dataOfs = datanode[nodes[i]].v.vi;
			if (dataOfs) {
				ReadDSGONodeType4(big_endian, buffer, dataOfs, datanode, nodes[i], xmlNode);
			}
			ReadDSGONodeSetNodeName(names, i, xmlNode);
			//xmlNode->SetText(datanode[nodes[i]].pos);
			break;
		}
		default:
			xmlNode = header->InsertNewChildElement("error");
			break;
		}

		//xmlNode->SetAttribute("testSN", nodes[i]);
		// check for duplicates
		if (datanode[nodes[i]].readCount) {
			xmlNode->SetAttribute("reusable", datanode[nodes[i]].readCount);
		}
		// node read count + 1
		datanode[nodes[i]].readCount += 1;
		// check loaded by type4
		if (datanode[nodes[i]].readByType4) {
			xmlNode->SetAttribute("MARKNAME", datanode[nodes[i]].nodeMark.c_str());
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
			xmlVNode = xmlNode->InsertNewChildElement("val");
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
			xmlVNode = xmlNode->InsertNewChildElement("LoadNode");
			int nodeIndex = ReadInt32(&buffer[curPos + 4], big_endian);
			xmlVNode->SetText(datanode[nodeIndex].nodeMark.c_str());
			startPos += 8;
			break;
		}
		case 4: {
			// 4 is complex arithmetic and needs more information
			xmlVNode = xmlNode->InsertNewChildElement("arithmetic");
			int to4type = ReadInt32(&buffer[curPos + 4], big_endian);
			switch (to4type) {
			case -2147483648: {
				// pow(v1, v2)
				xmlVNode->SetAttribute("instruction", "fpow");
				break;
			}
			case -2147483647: {
				xmlVNode->SetAttribute("instruction", "flog2");
				break;
			}
			case -2147483646: {
				// if +0>+28h, +0 is +28h
				xmlVNode->SetAttribute("instruction", "fget_L");
				break;
			}
			case -2147483645: {
				// if +28h>+0, +28h is +8
				xmlVNode->SetAttribute("instruction", "fget_B");
				break;
			}
			case -2147483644: {
				// take the biggest of the last 3 values
				xmlVNode->SetAttribute("instruction", "fmax3");
				break;
			}
			case -2147483643: {
				// limit the last value to 0.0 - 1.0
				xmlVNode->SetAttribute("instruction", "fgetM1");
				break;
			}
			case -2147483642: {
				// v2 - v1 * v3 + v1
				xmlVNode->SetAttribute("instruction", "fsma");
				break;
			}
			case -2147483641: {
				// sin(v1)
				xmlVNode->SetAttribute("instruction", "fsin");
				break;
			}
			case -2147483640: {
				// cos(v1)
				xmlVNode->SetAttribute("instruction", "fcos");
				break;
			}
			case -2147483639: {
				// tan(v1)
				xmlVNode->SetAttribute("instruction", "ftan");
				break;
			}
			case -2147483638: {
				// asin(v1)
				xmlVNode->SetAttribute("instruction", "fasin");
				break;
			}
			case -2147483637: {
				// acos(v1)
				xmlVNode->SetAttribute("instruction", "facos");
				break;
			}
			case -2147483636: {
				// atan(v1)
				xmlVNode->SetAttribute("instruction", "fatan");
				break;
			}
			default:
				break;
			}

			startPos += 8;
			break;
		}
		case 5: {
			xmlVNode = xmlNode->InsertNewChildElement("arithmetic");
			xmlVNode->SetAttribute("instruction", "add");
			startPos += 4;
			break;
		}
		case 6: {
			xmlVNode = xmlNode->InsertNewChildElement("arithmetic");
			xmlVNode->SetAttribute("instruction", "sub");
			startPos += 4;
			break;
		}
		case 7: {
			xmlVNode = xmlNode->InsertNewChildElement("arithmetic");
			xmlVNode->SetAttribute("instruction", "mul");
			startPos += 4;
			break;
		}
		case 8: {
			xmlVNode = xmlNode->InsertNewChildElement("arithmetic");
			xmlVNode->SetAttribute("instruction", "div");
			startPos += 4;
			break;
		}
		case 9: {
			xmlVNode = xmlNode->InsertNewChildElement("arithmetic");
			xmlVNode->SetAttribute("instruction", "mod");
			startPos += 4;
			break;
		}
		case 10: {
			xmlVNode = xmlNode->InsertNewChildElement("arithmetic");
			xmlVNode->SetAttribute("instruction", "shl");
			startPos += 4;
			break;
		}
		case 11: {
			xmlVNode = xmlNode->InsertNewChildElement("arithmetic");
			xmlVNode->SetAttribute("instruction", "shr");
			startPos += 4;
			break;
		}
		case 12: {
			xmlVNode = xmlNode->InsertNewChildElement("arithmetic");
			xmlVNode->SetAttribute("instruction", "and");
			startPos += 4;
			break;
		}
		case 13: {
			xmlVNode = xmlNode->InsertNewChildElement("arithmetic");
			xmlVNode->SetAttribute("instruction", "or");
			startPos += 4;
			break;
		}
		default:
			startPos += 4;
			break;
		}
	}
}
