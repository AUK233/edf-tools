#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <algorithm>

#include "util.h"
#include "RMPA6.h"

#define DEBUGMODE

void RMPA6::Read(const std::wstring& path)
{
	std::ifstream file(path + L".rmpa", std::ios::binary | std::ios::ate | std::ios::in);

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);

	int isSuccess = 0;
	if (file.read(buffer.data(), size)) {
		isSuccess = 1;
	}
	file.close();

	if (isSuccess)
	{
		// get header
		ReadHeader(buffer);

		// check version
		if (*(int*)&buffer[header.cameraOffset] != -1) {
			std::wcout << L"This is not an EDF6 RMPA!\n";
		}
		else {
			// create xml
			tinyxml2::XMLDocument xml;
			xml.InsertFirstChild(xml.NewDeclaration());
			tinyxml2::XMLElement* xmlHeader = xml.NewElement("RMPA");
			xml.InsertEndChild(xmlHeader);
			xmlHeader->SetAttribute("version", "6");

			// read data
			DataNodeCount = 4;
			ReadRouteNode(buffer, xmlHeader);
			ReadShapeNode(buffer, xmlHeader);
			tinyxml2::XMLElement* xmlCamera = xmlHeader->InsertNewChildElement("Camera");
			xmlCamera->SetAttribute("note", "this block is not supported");
			ReadPointNode(buffer, xmlHeader);

#if defined(DEBUGMODE)
			xmlHeader->SetAttribute("DataCount", DataNodeCount);
#endif

			std::string outfile = WideToUTF8(path) + "_RMPA.xml";
			xml.SaveFile(outfile.c_str());
		}
		// end
	}

	//Clear buffers
	buffer.clear();
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

	if (pNode->pad14[0] || pNode->pad14[1] || pNode->pad14[2]) {
		std::wcout << L"Current value is not 0:" + ToString(pos + 0x14) + L"\n";
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

void RMPA6::ReadStringToXml(const std::vector<char>& buffer, int pos, tinyxml2::XMLElement* xmlNode, const char* name)
{
	std::wstring wstr;
	std::string utf8str;
	if (*(INT16*)&buffer[pos]) {
		wstr = ReadUnicode(buffer, pos, IsBigEndian);
		utf8str = WideToUTF8(wstr);
		xmlNode->SetAttribute(name, utf8str.c_str());
	}
}

void RMPA6::ReadValue_SetFloat3(tinyxml2::XMLElement* xmlNode, const float* vf)
{
	xmlNode->SetAttribute("x", vf[0]);
	xmlNode->SetAttribute("y", vf[1]);
	xmlNode->SetAttribute("z", vf[2]);
}

void RMPA6::ReadRouteNode(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader)
{
	inWPNode_t n_Main;
	int ofs_Main = header.routeOffset;
	ReadRouteNode(buffer, ofs_Main, &n_Main);
	tinyxml2::XMLElement* xmlMainNode = xmlHeader->InsertNewChildElement("Route");
	ReadName(buffer, ofs_Main + n_Main.nameOffset, xmlMainNode);
	// next is unknown value
#if defined(DEBUGMODE)
	xmlMainNode->SetAttribute("pad0", n_Main.pad0);
	xmlMainNode->SetAttribute("nameSize", n_Main.nameSize);
#endif

	// read waypoint data
	std::vector<outRouteNode_t> v_waypoint(n_Main.waypointCount);
	// get node offset
	int ofs_node = ofs_Main + n_Main.waypointOffset;
	for (int i = 0; i < n_Main.waypointCount; i++) {
		ReadRouteNode(buffer, ofs_node, &v_waypoint[i].in);
		v_waypoint[i].pos = ofs_node;
		v_waypoint[i].str_node = "WP_" + std::to_string(i);
		DataNodeCount++;

#if defined(DEBUGMODE)
		if (v_waypoint[i].in.index != i) {
			std::wcout << L"The index is different in: " + ToString(ofs_node) + L"\n";
		}
		else if (v_waypoint[i].in.index >= n_Main.waypointCount) {
			std::wcout << L"Too large an index value in: " + ToString(ofs_node) + L"\n";
		}
#endif

		ofs_node += 0x30;
	}


	std::vector<inWPSubNode_t> v_Node(n_Main.subNodeCount);
	tinyxml2::XMLElement* xmlNode;
	tinyxml2::XMLElement* xmlData;
	tinyxml2::XMLElement* xmlValue;
	ofs_node = ofs_Main + n_Main.subOffset;
	for (int i = 0; i < n_Main.subNodeCount; i++) {
		ReadRouteSubNode(buffer, ofs_node, &v_Node[i]);
		xmlNode = xmlMainNode->InsertNewChildElement("Node");
		ReadName(buffer, ofs_node + v_Node[i].nameOffset, xmlNode);
		// next is unknown value
#if defined(DEBUGMODE)
		tinyxml2::XMLElement* xmlDebug = xmlNode->InsertNewChildElement("Debug");
		xmlDebug->SetAttribute("pos", ofs_node);
		xmlDebug->SetAttribute("pad0", v_Node[i].pad0);
		xmlDebug->SetAttribute("nameSize", v_Node[i].nameSize);
		xmlNode->SetAttribute("dataPos", ofs_node + v_Node[i].dataStartOffset);
#endif

		// get data offset
		int WPindex = 0;
		int ofs_data = ofs_node + v_Node[i].indexOffset;
		for (int j = 0; j < v_Node[i].indexCount; j++) {
			WPindex = ReadInt32(&buffer[ofs_data], 1);

			if (WPindex < v_waypoint.size()) {
				int curpos = v_waypoint[WPindex].pos;
				xmlData = xmlNode->InsertNewChildElement("Data");
				xmlData->SetAttribute("wpname", v_waypoint[WPindex].str_node.c_str());
				ReadName(buffer, curpos + v_waypoint[WPindex].in.nameOffset, xmlData);

#if defined(DEBUGMODE)
				xmlDebug = xmlData->InsertNewChildElement("Debug");
				xmlDebug->SetAttribute("pos", curpos);
				xmlDebug->SetAttribute("index", v_waypoint[WPindex].in.index);
				xmlDebug->SetAttribute("IndexID", v_waypoint[WPindex].in.IndexID);
				xmlDebug->SetAttribute("nameSize", v_waypoint[WPindex].in.nameSize);
				xmlDebug->SetAttribute("WPCount", v_waypoint[WPindex].in.WPCount);
#endif

				//
				xmlValue = xmlData->InsertNewChildElement("position");
				ReadValue_SetFloat3(xmlValue, v_waypoint[WPindex].in.pos);
				
				// get linked waypoint
				int ofs_sub = curpos + v_waypoint[WPindex].in.WPOffset;
				xmlValue = xmlData->InsertNewChildElement("linkedroute");
#if defined(DEBUGMODE)
				xmlValue->SetAttribute("pos", ofs_sub);
#endif
				for (int k = 0; k < v_waypoint[WPindex].in.WPCount; k++) {
					int IndexNext = ReadInt32(&buffer[ofs_sub], 1);
					xmlValue->InsertNewChildElement("wpname")->SetText(v_waypoint[IndexNext].str_node.c_str());
					ofs_sub += 4;
				}

				// next is info
				// it does not calculate offset from start
				ofs_sub = curpos + 0x28 + v_waypoint[WPindex].in.infoOffset;
				inRMPAInfo_t shapeInfo;
				for (int k = 0; k < v_waypoint[WPindex].in.infoCount; k++) {
					// it is little-endian!
					memcpy(&shapeInfo, &buffer[ofs_sub], 8);
					xmlValue = xmlData->InsertNewChildElement("info");
#if defined(DEBUGMODE)
					xmlValue->SetAttribute("pos", ofs_sub);
#endif
					ReadStringToXml(buffer, ofs_sub + shapeInfo.titleOffset, xmlValue, "title");
					ReadStringToXml(buffer, ofs_sub + shapeInfo.contentOffset, xmlValue, "content");

					ofs_sub += 8;
				}
			}
			else {
				std::wcout << L"Index exceeds total in: " + ToString(ofs_data) + L"\n";
			}
			// end
			ofs_data += 4;
		}

		ofs_node += 0x18;
	}
}

void RMPA6::ReadRouteNode(const std::vector<char>& buffer, int pos, inWPNode_t* pNode)
{
	if (IsBigEndian) {
		int* temp = (int*)pNode;
		static_assert(sizeof(inWPNode_t) == 8 * 4, "size is error.");
		int curpos = pos;
		for (int i = 0; i < 8; i++) {
			temp[i] = ReadInt32(&buffer[curpos], 1);
			curpos += 4;
		}
	}
	else {
		memcpy(pNode, &buffer[pos], 0x20);
	}

	if (pNode->pad1c) {
		std::wcout << L"Current value is not 0:" + ToString(pos + 0x1C) + L"\n";
	}
}

void RMPA6::ReadRouteNode(const std::vector<char>& buffer, int pos, inRouteNode_t* pNode)
{
	if (IsBigEndian) {
		int* temp = (int*)pNode;
		static_assert(sizeof(inRouteNode_t) == 12 * 4, "size is error.");
		int curpos = pos;
		for (int i = 0; i < 12; i++) {
			temp[i] = ReadInt32(&buffer[curpos], 1);
			curpos += 4;
		}
	}
	else {
		memcpy(pNode, &buffer[pos], 0x30);
	}
}

void RMPA6::ReadRouteSubNode(const std::vector<char>& buffer, int pos, inWPSubNode_t* pNode)
{
	if (IsBigEndian) {
		int* temp = (int*)pNode;
		static_assert(sizeof(inWPSubNode_t) == 6 * 4, "size is error.");
		int curpos = pos;
		for (int i = 0; i < 6; i++) {
			temp[i] = ReadInt32(&buffer[curpos], 1);
			curpos += 4;
		}
	}
	else {
		memcpy(pNode, &buffer[pos], 0x18);
	}
}

void RMPA6::ReadShapeNode(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader)
{
	inNode_t n_Main;
	int ofs_Main = header.shapeOffset;
	ReadNode(buffer, ofs_Main, &n_Main);
	tinyxml2::XMLElement* xmlMainNode = xmlHeader->InsertNewChildElement("Shape");
	ReadName(buffer, ofs_Main + n_Main.nameOffset, xmlMainNode);
	// next is unknown value
#if defined(DEBUGMODE)
	xmlMainNode->SetAttribute("pad0", n_Main.pad0);
	xmlMainNode->SetAttribute("nameSize", n_Main.nameSize);
#endif

	std::vector<inSubNode_t> v_Node(n_Main.subNodeCount);
	tinyxml2::XMLElement* xmlNode;
	tinyxml2::XMLElement* xmlData;
	tinyxml2::XMLElement* xmlValue;
	// get node offset
	int ofs_node = ofs_Main + n_Main.subOffset;
	for (int i = 0; i < n_Main.subNodeCount; i++) {
		ReadSubNode(buffer, ofs_node, &v_Node[i]);
		xmlNode = xmlMainNode->InsertNewChildElement("Node");
		ReadName(buffer, ofs_node + v_Node[i].nameOffset, xmlNode);
		// next is unknown value
#if defined(DEBUGMODE)
		tinyxml2::XMLElement* xmlDebug = xmlNode->InsertNewChildElement("Debug");
		xmlDebug->SetAttribute("pos", ofs_node);
		xmlDebug->SetAttribute("pad0", v_Node[i].pad0);
		xmlDebug->SetAttribute("nameSize", v_Node[i].nameSize);
#endif

		int SubNodeCount = v_Node[i].subNodeCount;
		std::vector<inShapeNode_t> v_SubNode(SubNodeCount);
		// get data offset
		int ofs_data = ofs_node + v_Node[i].subOffset;
		for (int j = 0; j < SubNodeCount; j++) {
			ReadShapeNode(buffer, ofs_data, &v_SubNode[j]);
			if (v_SubNode[j].dataCount != 1 || v_SubNode[j].infoCount != 1) {
				std::wcout << L"Found different shape data value in: " + ToString(ofs_data + 20) + L"\n";
			}
			//
			xmlData = xmlNode->InsertNewChildElement("Data");
			ReadStringToXml(buffer, ofs_data + v_SubNode[j].typeOffset, xmlData, "type");
			ReadName(buffer, ofs_data + v_SubNode[j].nameOffset, xmlData);
			// next is unknown value
#if defined(DEBUGMODE)
			xmlDebug = xmlData->InsertNewChildElement("Debug");
			xmlDebug->SetAttribute("pos", ofs_data);
			xmlDebug->SetAttribute("IndexID", v_SubNode[j].IndexID);
			xmlDebug->SetAttribute("nameSize", v_SubNode[j].nameSize);
			xmlDebug->SetAttribute("typeSize", v_SubNode[j].typeSize);
#endif

			// next is data
			int ofs_sub = ofs_data + v_SubNode[j].dataOffset;
			inShapeData_t shapeData;
			ReadShapeNodeData(buffer, ofs_sub, &shapeData);
			if (shapeData.pad38) {
				std::wcout << L"(shape data) Found different value in: " + ToString(ofs_sub) + L"\n";
			}
			//
			xmlValue = xmlData->InsertNewChildElement("position");
			ReadValue_SetFloat3(xmlValue, shapeData.pos);
			xmlValue = xmlData->InsertNewChildElement("vector1");
			ReadValue_SetFloat3(xmlValue, shapeData.vec1);
			xmlValue = xmlData->InsertNewChildElement("vector2");
			ReadValue_SetFloat3(xmlValue, shapeData.vec2);
			xmlValue = xmlData->InsertNewChildElement("other");
#if defined(DEBUGMODE)
			xmlValue->SetAttribute("pos", ofs_sub);
#endif
			xmlValue->SetAttribute("radius", shapeData.radius);
			xmlValue->SetAttribute("height", shapeData.height);

			// next is info
			// it does not calculate offset from start
			ofs_sub = ofs_data + 0x1C + v_SubNode[j].infoOffset;
			inRMPAInfo_t shapeInfo;
			for (int k = 0; k < v_SubNode[j].infoCount; k++) {
				// it is little-endian!
				memcpy(&shapeInfo, &buffer[ofs_sub], 8);
				xmlValue = xmlData->InsertNewChildElement("info");
#if defined(DEBUGMODE)
				xmlValue->SetAttribute("pos", ofs_sub);
#endif
				ReadStringToXml(buffer, ofs_sub + shapeInfo.titleOffset, xmlValue, "title");
				ReadStringToXml(buffer, ofs_sub + shapeInfo.contentOffset, xmlValue, "content");

				ofs_sub += 8;
			}

			// end
			ofs_data += 0x24;
			DataNodeCount++;
		}

		ofs_node += 0x14;
	}
}

void RMPA6::ReadShapeNode(const std::vector<char>& buffer, int pos, inShapeNode_t* pNode)
{
	if (IsBigEndian) {
		int* temp = (int*)pNode;
		static_assert(sizeof(inShapeNode_t) == 9 * 4, "size is error.");
		int curpos = pos;
		for (int i = 0; i < 9; i++) {
			temp[i] = ReadInt32(&buffer[curpos], 1);
			curpos += 4;
		}
	}
	else {
		memcpy(pNode, &buffer[pos], 0x24);
	}
}

void RMPA6::ReadShapeNodeData(const std::vector<char>& buffer, int pos, inShapeData_t* pNode)
{
	if (IsBigEndian) {
		int* temp = (int*)pNode;
		static_assert(sizeof(inShapeData_t) == 16 * 4, "size is error.");
		int curpos = pos;
		for (int i = 0; i < 16; i++) {
			temp[i] = ReadInt32(&buffer[curpos], 1);
			curpos += 4;
		}
	}
	else {
		memcpy(pNode, &buffer[pos], 0x40);
	}
}

void RMPA6::ReadPointNode(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader)
{
	inNode_t n_Main;
	int ofs_Main = header.pointOffset;
	ReadNode(buffer, ofs_Main, &n_Main);
	tinyxml2::XMLElement* xmlMainNode = xmlHeader->InsertNewChildElement("Point");
	ReadName(buffer, ofs_Main + n_Main.nameOffset, xmlMainNode);
	// next is unknown value
#if defined(DEBUGMODE)
	xmlMainNode->SetAttribute("pad0", n_Main.pad0);
	xmlMainNode->SetAttribute("nameSize", n_Main.nameSize);
#endif

	std::vector<inSubNode_t> v_Node(n_Main.subNodeCount);
	tinyxml2::XMLElement* xmlNode;
	tinyxml2::XMLElement* xmlData;
	tinyxml2::XMLElement* xmlValue;
	// get node offset
	int ofs_node = ofs_Main + n_Main.subOffset;

	for (int i = 0; i < n_Main.subNodeCount; i++) {
		ReadSubNode(buffer, ofs_node, &v_Node[i]);
		xmlNode = xmlMainNode->InsertNewChildElement("Node");
		ReadName(buffer, ofs_node + v_Node[i].nameOffset, xmlNode);
		// next is unknown value
#if defined(DEBUGMODE)
		tinyxml2::XMLElement* xmlDebug = xmlNode->InsertNewChildElement("Debug");
		xmlDebug->SetAttribute("pos", ofs_node);
		xmlDebug->SetAttribute("pad0", v_Node[i].pad0);
		xmlDebug->SetAttribute("nameSize", v_Node[i].nameSize);
#endif

		int SubNodeCount = v_Node[i].subNodeCount;
		std::vector<inPointNode_t> v_SubNode(SubNodeCount);
		// get data offset
		int ofs_data = ofs_node + v_Node[i].subOffset;

		for (int j = 0; j < SubNodeCount; j++) {
			ReadPointNode(buffer, ofs_data, &v_SubNode[j]);
			xmlData = xmlNode->InsertNewChildElement("Data");
			ReadName(buffer, ofs_data + v_SubNode[j].data.nameOffset, xmlData);
			xmlValue = xmlData->InsertNewChildElement("position");
			ReadValue_SetFloat3(xmlValue, v_SubNode[j].pos1);
			xmlValue = xmlData->InsertNewChildElement("orientation");
			ReadValue_SetFloat3(xmlValue, v_SubNode[j].pos2);
			// next is unknown value
#if defined(DEBUGMODE)
			xmlDebug = xmlData->InsertNewChildElement("Debug");
			xmlDebug->SetAttribute("pos", ofs_data);
			xmlDebug->SetAttribute("IndexID", v_SubNode[j].IndexID);
			xmlDebug->SetAttribute("pad10", v_SubNode[j].pad10);
			xmlDebug->SetAttribute("pad20", v_SubNode[j].data.pad0);
			xmlDebug->SetAttribute("nameSize", v_SubNode[j].data.nameSize);
			xmlDebug->SetAttribute("pad2C", v_SubNode[j].data.subNodeCount);
			xmlDebug->SetAttribute("pad30", v_SubNode[j].data.subOffset);
#endif
			// end
			ofs_data += 0x34;
			DataNodeCount++;
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
