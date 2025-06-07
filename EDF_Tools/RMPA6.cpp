#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <vector>
#include <algorithm>

#include "util.h"
#include "RMPA6.h"

#define DEBUGMODE

// here is raw data to write
// ================================================================================================
static const BYTE RAW_Header[48] = {
	0x00, 0x50, 0x4D, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xC1,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xC2, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xC3,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xC4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const BYTE RAW_MainNode[32] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC1, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// ================================================================================================


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

			std::wcout << L" Reading route node......\n";
			ReadRouteNode(buffer, xmlHeader);

			std::wcout << L" Reading shape node......\n";
			ReadShapeNode(buffer, xmlHeader);

			std::wcout << L" Skip camera node.\n";
			tinyxml2::XMLElement* xmlCamera = xmlHeader->InsertNewChildElement("Camera");
			xmlCamera->SetAttribute("note", "this block is not supported");

			std::wcout << L" Reading point node......\n";
			ReadPointNode(buffer, xmlHeader);

#if defined(DEBUGMODE)
			xmlHeader->SetAttribute("DataCount", DataNodeCount);
#endif

			std::string outfile = WideToUTF8(path) + "_RMPA.xml";
			xml.SaveFile(outfile.c_str());
			std::wcout << L"Done!\n";
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
			if (v_SubNode[j].dataCount != 1) {
				std::wcout << L"Found different shape data value in: " + ToString(ofs_data + 20) + L"\n";
			}

			if (v_SubNode[j].infoCount != 1) {
				std::wcout << L"Found different shape info value in: " + ToString(ofs_data + 28) + L"\n";
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
			ReadName(buffer, ofs_data + v_SubNode[j].nameOffset, xmlData);
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
			xmlDebug->SetAttribute("pad20", v_SubNode[j].pad20);
			xmlDebug->SetAttribute("nameSize", v_SubNode[j].nameSize);
			xmlDebug->SetAttribute("pad2C", v_SubNode[j].infoCount);
			xmlDebug->SetAttribute("pad30", v_SubNode[j].infoOffset);
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

// ==================================================================================================

void RMPA6::Write(const std::wstring& path)
{
	std::wstring sourcePath = path + L"_rmpa.xml";
	std::wcout << "Will output RMPA file.\n";
	std::string UTF8Path = WideToUTF8(sourcePath);


	tinyxml2::XMLDocument doc;
	doc.LoadFile(UTF8Path.c_str());

	tinyxml2::XMLElement* header = doc.FirstChildElement("RMPA");
	std::vector< char > bytes = WriteData(header);

	// Final write.
	/**/
	std::ofstream newFile(path + L".rmpa", std::ios::binary | std::ios::out | std::ios::ate);
	newFile.write(bytes.data(), bytes.size());
	newFile.close();

	std::wcout << L"Conversion completed: " + path + L".rmpa\n";
}

std::vector<char> RMPA6::WriteData(tinyxml2::XMLElement* xmlData)
{
	// initialization default
	std::vector<char> buffer(std::begin(RAW_Header), std::end(RAW_Header));
	v_wstring.resize(2, 0);
	std::vector<char> bytes;
	DataNodeCount = 0;
	int bufferSize;

	// read route
	std::wcout << L" Reading route node......\n";
	WriteBEdword(&buffer[0xC], 0x30);
	bytes = WriteRoute(xmlData->FirstChildElement("Route"), 0x30);
	buffer.insert(buffer.end(), bytes.begin(), bytes.end());

	// read shape
	std::wcout << L" Reading shape node......\n";
	bufferSize = buffer.size();
	WriteBEdword(&buffer[0x14], bufferSize);
	bytes = WriteShape(xmlData->FirstChildElement("Shape"), bufferSize);
	buffer.insert(buffer.end(), bytes.begin(), bytes.end());

	// read camera
	bufferSize = buffer.size();
	WriteBEdword(&buffer[0x1C], bufferSize);
	bytes = WriteCamera(xmlData->FirstChildElement("Camera"), bufferSize);
	buffer.insert(buffer.end(), bytes.begin(), bytes.end());

	// read point
	std::wcout << L" Reading point node......\n";
	bufferSize = buffer.size();
	WriteBEdword(&buffer[0x24], bufferSize);
	bytes = WritePoint(xmlData->FirstChildElement("Point"), bufferSize);
	buffer.insert(buffer.end(), bytes.begin(), bytes.end());

	// update string offset
	bufferSize = buffer.size();
	int str_curpos;
	BEtoLEByte_t str_curofs;
	for (int i = 0; i < v_update_string.size(); i++) {
		str_curpos = v_update_string[i].pos;
		str_curofs.s32 = v_update_string[i].offset + bufferSize;
		WriteBEdword(&buffer[str_curpos], str_curofs.u32);
	}
	// LE string offset
	for (int i = 0; i < v_updateLE_string.size(); i++) {
		str_curpos = v_updateLE_string[i].pos;
		str_curofs.s32 = v_updateLE_string[i].offset + bufferSize;
		WriteINT32LE(&buffer[str_curpos], str_curofs.s32);
	}

	// write string
	buffer.insert(buffer.end(), v_wstring.begin(), v_wstring.end());
	v_wstring.clear();

	// end
	return buffer;
}

void RMPA6::WriteBEdwordGroup(char* pdata, UINT32* pINT, int count)
{
	for (int i = 0; i < count; i++) {
		WriteBEdword(&pdata[i * 4], pINT[i]);
	}
}

int RMPA6::WriteWideStringData(const std::wstring& in)
{
	// check map
	auto it = map_wstring.find(in);
	if (it != map_wstring.end()) {
		return it->second;
	}
	// write string
	int offset = v_wstring.size();
	map_wstring[in] = offset;
	WriteWideStringToBuffer(in);
	return offset;
}

void RMPA6::WriteWideStringToBuffer(const std::wstring& in)
{
	auto strnBytes = reinterpret_cast<const char*>(&in[0]);
	int size = in.size() * 2;

	for (int i = 0; i < size; i += 2)
	{
		v_wstring.push_back(strnBytes[i + 1]);
		v_wstring.push_back(strnBytes[i]);
	}
	// Zero terminate
	v_wstring.push_back(0);
	v_wstring.push_back(0);
}

int RMPA6::WriteCommonWideString(tinyxml2::XMLElement* xmlData, const char* name, char* buffer, int baseSize)
{
	auto pName = xmlData->Attribute(name);
	if (!pName) {
		return baseSize;
	}

	std::string str = pName;
	if(str.empty()) {
		return baseSize;
	}

	std::wstring wstr = UTF8ToWide(str);
	BEtoLEByte_t str_size;
	str_size.s32 = wstr.size();
	WriteBEdword(buffer, str_size.u32);

	int offset = WriteWideStringData(wstr);
	offset += baseSize;
	return offset;
}

void RMPA6::WriteFloat4FromXML(tinyxml2::XMLElement* xmlData, float* p)
{
	p[0] = xmlData->FloatAttribute("x");
	p[1] = xmlData->FloatAttribute("y");
	p[2] = xmlData->FloatAttribute("z");
	p[3] = 0;
}

std::vector<char> RMPA6::WriteRoute(tinyxml2::XMLElement* xmlData, int inSize)
{
	// initialization
	std::vector<char> buffer(std::begin(RAW_MainNode), std::end(RAW_MainNode));
	DataNodeCount++;

	// get name
	updateDataOffset_t out_dataup;
	out_dataup.pos = inSize + 8;
	out_dataup.offset = WriteCommonWideString(xmlData, "name", &buffer[4], -inSize);
	v_update_string.push_back(out_dataup);

	// set temp
	std::vector<char> v_temp;
	std::vector<updateDataOffset_t> update_string, update_data;
	tinyxml2::XMLElement* entry;

	// read node
	std::vector<char> v_node;
	RMPA6VectorInt v_indexData;
	std::vector<tinyxml2::XMLElement*> v_xmlNode;
	int i_nodeCount = 0;
	for (entry = xmlData->FirstChildElement("Node"); entry != 0; entry = entry->NextSiblingElement("Node"))
	{
		// set initial
		v_temp.resize(0x18);
		WriteINT32LE(&v_temp[0], -1);
		WriteINT32LE(&v_temp[4], 0);
		// get name
		int nodePos = inSize + v_node.size();
		out_dataup.pos = nodePos + 8;
		out_dataup.offset = WriteCommonWideString(entry, "name", &v_temp[4], -nodePos);
		update_string.push_back(out_dataup);
		// set data info pos
		out_dataup.pos = v_node.size();
		out_dataup.offset = i_nodeCount;
		update_data.push_back(out_dataup);

		// read data node
		v_indexData.push_back(WriteRouteData_GetXML(entry, v_xmlNode));

		// end
		v_node.insert(v_node.end(), v_temp.begin(), v_temp.end());
		v_temp.clear();
		i_nodeCount++;
	}
	WriteBEdword(&buffer[0xC], i_nodeCount);

	// check 16-byte alignment
	int align = v_node.size() % 16;
	if (align) {
		for (int i = align; i < 16; i++) {
			v_node.push_back(0);
		}
	}
	// write node index
	std::vector<char> v_nodeIndex;
	for (int i = 0; i < update_data.size(); i++) {
		int curpos = update_data[i].pos;
		std::vector<int> temp_Index = v_indexData[update_data[i].offset];

		BEtoLEByte_t index_count;
		index_count.s32 = temp_Index.size();
		BEtoLEByte_t index_pos;
		index_pos.s32 = v_node.size() - curpos + v_nodeIndex.size();

		WriteBEdword(&v_node[curpos + 0xC], index_count.u32);
		WriteBEdword(&v_node[curpos + 0x10], index_pos.u32);

		int index_size = index_count.s32 * 4;
		v_temp.resize(index_size);
		WriteBEdwordGroup(v_temp.data(), (UINT32*)temp_Index.data(), index_count.s32);

		align = v_temp.size() % 16;
		if (align) {
			for (int j = align; j < 16; j++) {
				v_temp.push_back(0);
			}
		}

		v_nodeIndex.insert(v_nodeIndex.end(), v_temp.begin(), v_temp.end());
		v_temp.clear();
	}
	v_node.insert(v_node.end(), v_nodeIndex.begin(), v_nodeIndex.end());
	v_nodeIndex.clear();

	// read data node
	std::vector<char> v_data;
	WriteRouteData_GetData(v_xmlNode, v_data, 0x50);
	// write data header
	WriteBEdword(&buffer[0x14], v_xmlNode.size());
	WriteBEdword(&buffer[0x18], 0x20);
	v_xmlNode.clear();

	// update offset information
	int i_dataSize = v_data.size();
	for (int i = 0; i < update_data.size(); i++) {
		// data offset
		int i_nodePos = update_data[i].pos;
		BEtoLEByte_t i_dataOfs;
		i_dataOfs.s32 = i_nodePos + i_dataSize;
		i_dataOfs.s32 = -i_dataOfs.s32;
		WriteBEdword(&v_node[i_nodePos + 0x14], i_dataOfs.u32);

		// name offset
		int i_blockSize = i_dataSize + 0x20;
		update_string[i].pos += i_blockSize;
		update_string[i].offset -= i_blockSize;
	}
	v_indexData.clear();
	update_data.clear();

	// merge data
	buffer.insert(buffer.end(), v_data.begin(), v_data.end());
	v_data.clear();
	WriteBEdword(&buffer[0x10], buffer.size());
	//
	buffer.insert(buffer.end(), v_node.begin(), v_node.end());
	v_node.clear();
	//
	v_update_string.insert(v_update_string.end(), update_string.begin(), update_string.end());
	update_string.clear();

	return buffer;
}

std::vector<int> RMPA6::WriteRouteData_GetXML(tinyxml2::XMLElement* xmlData, std::vector<tinyxml2::XMLElement*>& v_xmlNode)
{
	std::vector<int> out;
	tinyxml2::XMLElement* entry;
	for (entry = xmlData->FirstChildElement("Data"); entry != 0; entry = entry->NextSiblingElement("Data")) {
		int index = v_xmlNode.size();
		out.push_back(index);
		v_xmlNode.push_back(entry);
	}
	return out;
}

void RMPA6::WriteRouteData_GetData(std::vector<tinyxml2::XMLElement*>& v_xmlNode, std::vector<char>& v_data, int baseSize)
{
	StringToIntMap map_string;
	std::vector<outRouteNode_t> v_out_data;

	tinyxml2::XMLElement* entry;
	outRouteNode_t out_data;

	// First, get index mapping
	for (int i = 0; i < v_xmlNode.size(); i++) {
		//add it first, then read it.
		DataNodeCount++;

		out_data.in.index = i;
		out_data.in.IndexID = DataNodeCount;
		out_data.pos = i * 0x30;

		// check mapping
		entry = v_xmlNode[i];
		auto pName = entry->Attribute("wpname");
		if (!pName) {
			goto pushData;
		}

		out_data.str_node = pName;
		if (out_data.str_node.empty()) {
			goto pushData;
		}

		map_string[out_data.str_node] = i;

		pushData:
		v_out_data.push_back(out_data);
	}

	// get node data
	int dataSize = v_xmlNode.size() * 0x30;
	v_data.resize(dataSize);
	std::vector<char> v_node_data;

	updateDataOffset_t out_dataup;
	tinyxml2::XMLElement* xmlNode, *xmlEntry;
	for (int i = 0; i < v_xmlNode.size(); i++) {
		entry = v_xmlNode[i];
		auto pNode = &v_out_data[i];

		// get name
		int nodePos = baseSize + pNode->pos;
		out_dataup.pos = nodePos + 0x14;
		out_dataup.offset = WriteCommonWideString(entry, "name", &v_data[pNode->pos + 0x10], -nodePos);
		v_update_string.push_back(out_dataup);

		// get coordinate
		xmlNode = entry->FirstChildElement("position");
		WriteFloat4FromXML(xmlNode, pNode->in.pos);

		// get linked waypoint
		pNode->in.WPOffset = dataSize - pNode->pos + v_node_data.size();
		std::vector<char> v_linked = WriteRouteData_GetLinkedWP(entry, pNode, map_string);
		v_node_data.insert(v_node_data.end(), v_linked.begin(), v_linked.end());

		// get info data
		pNode->in.infoOffset = dataSize - pNode->pos - 0x28 + v_node_data.size();
		int infoDataPos = baseSize + dataSize + v_node_data.size();
		std::vector<char> v_info = WriteCommonInfoData(entry, &pNode->in.infoCount, infoDataPos);
		v_node_data.insert(v_node_data.end(), v_info.begin(), v_info.end());

		// update data
		WriteBEdwordGroup(&v_data[pNode->pos], (UINT32*)&pNode->in.index, 4);
		WriteBEdwordGroup(&v_data[pNode->pos + 0x18], (UINT32*)pNode->in.pos, 6);

		// end
	}

	// finally, merge data
	v_data.insert(v_data.end(), v_node_data.begin(), v_node_data.end());
	v_node_data.clear();
}

std::vector<char> RMPA6::WriteRouteData_GetLinkedWP(tinyxml2::XMLElement* entry, outRouteNode_t* pNode,
													const StringToIntMap& map_string)
{
	pNode->in.WPCount = 0;
	int nodeIndex = 0;
	std::vector<int> v_node_index;
	tinyxml2::XMLElement* xmlNode = entry->FirstChildElement("linkedroute");
	for (tinyxml2::XMLElement* xmlEntry = xmlNode->FirstChildElement("wpname"); xmlEntry != 0; xmlEntry = xmlEntry->NextSiblingElement("wpname")) {
		std::string wpname = xmlEntry->GetText();
		pNode->in.WPCount++;

		auto it = map_string.find(wpname);
		if (it != map_string.end()) {
			nodeIndex = it->second;
		}
		v_node_index.push_back(nodeIndex);
	}

	// check alignment
	int align = v_node_index.size() % 4;
	if (align) {
		for (int j = align; j < 4; j++) {
			v_node_index.push_back(0);
		}
	}

	// write to buffer
	int tempSize = v_node_index.size();
	std::vector<char> out(tempSize * 4);
	WriteBEdwordGroup(&out[0], (UINT32*)&v_node_index[0], tempSize);

	v_node_index.clear();
	return out;
}

std::vector<char> RMPA6::WriteShape(tinyxml2::XMLElement* xmlData, int inSize)
{
	// initialization
	std::vector<char> buffer(std::begin(RAW_MainNode), std::end(RAW_MainNode));
	DataNodeCount++;

	updateDataOffset_t out_dataup;
	out_dataup.pos = inSize + 8;
	out_dataup.offset = WriteCommonWideString(xmlData, "name", &buffer[4], -inSize);
	v_update_string.push_back(out_dataup);

	// set temp
	std::vector<char> v_temp;
	std::vector<updateDataOffset_t> update_data;
	tinyxml2::XMLElement* entry;

	// read node
	std::vector<char> v_node;
	std::vector<tinyxml2::XMLElement*> v_xmlNode;
	int i_nodeCount = 0;
	for (entry = xmlData->FirstChildElement("Node"); entry != 0; entry = entry->NextSiblingElement("Node")) {
		// set initial
		v_temp.resize(0x14);
		WriteINT32LE(&v_temp[0], -1);
		WriteINT32LE(&v_temp[4], 0);
		// get name
		int nodePos = inSize + v_node.size() + 0x20;
		out_dataup.pos = nodePos + 8;
		out_dataup.offset = WriteCommonWideString(entry, "name", &v_temp[4], -nodePos);
		v_update_string.push_back(out_dataup);
		// set data info pos
		out_dataup.pos = v_node.size();
		out_dataup.offset = i_nodeCount;
		update_data.push_back(out_dataup);

		// end
		v_xmlNode.push_back(entry);
		v_node.insert(v_node.end(), v_temp.begin(), v_temp.end());
		v_temp.clear();
		i_nodeCount++;
	}
	WriteBEdword(&buffer[0xC], i_nodeCount);
	WriteBEdword(&buffer[0x10], 0x20);
	// check 16-byte alignment
	int align = v_node.size() % 16;
	if (align) {
		for (int i = align; i < 16; i++) {
			v_node.push_back(0);
		}
	}

	int bufferSize = v_node.size() + 0x20 + inSize;
	// read data
	std::vector<char> v_data;
	BEtoLEByte_t dataNodeCount;
	for (int i = 0; i < v_xmlNode.size(); i++) {
		v_temp = WriteShapeData(v_xmlNode[i], bufferSize, &dataNodeCount.s32);
		int curpos = update_data[i].pos;
		WriteBEdword(&v_node[curpos + 0xC], dataNodeCount.u32);

		// here dataNodeCount as offset
		dataNodeCount.s32 = v_node.size() - curpos + v_data.size();
		WriteBEdword(&v_node[curpos + 0x10], dataNodeCount.u32);

		// write to buffer
		bufferSize += v_temp.size();
		v_data.insert(v_data.end(), v_temp.begin(), v_temp.end());
		v_temp.clear();
	}
	update_data.clear();
	v_xmlNode.clear();

	// merge data
	WriteBEdword(&buffer[0x10], 0x20);
	buffer.insert(buffer.end(), v_node.begin(), v_node.end());
	v_node.clear();
	//
	buffer.insert(buffer.end(), v_data.begin(), v_data.end());
	v_data.clear();

	return buffer;
}

std::vector<char> RMPA6::WriteShapeData(tinyxml2::XMLElement* xmlData, int baseSize, int* nodeCount)
{
	tinyxml2::XMLElement* entry;
	std::vector<tinyxml2::XMLElement*> v_xmlNode;
	outShapeNode_t out_node;
	std::vector<outShapeNode_t> v_out_node;

	// preprocess
	int dataCount = 0;
	for (entry = xmlData->FirstChildElement("Data"); entry != 0; entry = entry->NextSiblingElement("Data")) {
		v_xmlNode.push_back(entry);
		DataNodeCount++;

		out_node.in.IndexID = DataNodeCount;
		out_node.in.dataCount = 1;
		out_node.pos = dataCount * 0x24;
		v_out_node.push_back(out_node);

		dataCount++;
	}
	*nodeCount = dataCount;
	int dataSize = dataCount * 0x24;
	std::vector<char> out(dataSize);
	// check 16-byte alignment
	int align = dataSize % 16;
	if (align) {
		for (int i = align; i < 16; i++) {
			out.push_back(0);
		}
		dataSize = out.size();
	}

	// get data
	updateDataOffset_t out_dataup;
	std::vector<char> v_node_data;
	std::vector<char> v_temp(0x40);
	for (int i = 0; i < v_xmlNode.size(); i++) {
		auto pNode = &v_out_node[i];
		entry = v_xmlNode[i];

		// get type
		int nodePos = baseSize + pNode->pos;
		out_dataup.pos = nodePos + 4;
		out_dataup.offset = WriteCommonWideString(entry, "type", &out[pNode->pos], -nodePos);
		v_update_string.push_back(out_dataup);
		// get name
		out_dataup.pos = nodePos + 0xC;
		out_dataup.offset = WriteCommonWideString(entry, "name", &out[pNode->pos + 8], -nodePos);
		v_update_string.push_back(out_dataup);

		// get data
		inShapeData_t shapeData;
		auto xmlNode = entry->FirstChildElement("position");
		WriteFloat4FromXML(xmlNode, shapeData.pos);
		xmlNode = entry->FirstChildElement("vector1");
		WriteFloat4FromXML(xmlNode, shapeData.vec1);
		xmlNode = entry->FirstChildElement("vector2");
		WriteFloat4FromXML(xmlNode, shapeData.vec2);
		xmlNode = entry->FirstChildElement("other");
		shapeData.radius = xmlNode->FloatAttribute("radius");
		shapeData.height = xmlNode->FloatAttribute("height");
		shapeData.pad38 = 0;
		// write data
		pNode->in.dataOffset = dataSize - pNode->pos + v_node_data.size();
		WriteBEdwordGroup(v_temp.data(), (UINT32*)&shapeData, 16);
		v_node_data.insert(v_node_data.end(), v_temp.begin(), v_temp.end());

		// get info data
		pNode->in.infoOffset = dataSize - pNode->pos - 0x1C + v_node_data.size();
		int infoDataPos = baseSize + dataSize + v_node_data.size();
		std::vector<char> v_info = WriteCommonInfoData(entry, &pNode->in.infoCount, infoDataPos);
		v_node_data.insert(v_node_data.end(), v_info.begin(), v_info.end());

		// update data
		WriteBEdwordGroup(&out[pNode->pos + 0x10], (UINT32*)&pNode->in.IndexID, 5);
	}

	// finally, merge data
	out.insert(out.end(), v_node_data.begin(), v_node_data.end());
	return out;
}

std::vector<char> RMPA6::WriteCommonInfoData(tinyxml2::XMLElement* entry, int* pSize, int baseSize)
{
	std::vector<int> v_node_offset;

	std::string str;
	std::wstring wstr;
	updateDataOffset_t updata;

	int infoCount = 0;
	int curSize = baseSize;
	int offset;
	for (tinyxml2::XMLElement* xmlEntry = entry->FirstChildElement("info"); xmlEntry != 0; xmlEntry = xmlEntry->NextSiblingElement("info")) {
		// get title offset
		str = xmlEntry->Attribute("title");
		wstr = UTF8ToWide(str);
		offset = WriteWideStringData(wstr);
		//
		updata.pos = curSize;
		updata.offset = offset - curSize;
		v_updateLE_string.push_back(updata);
		curSize += 4;

		// get content offset
		str = xmlEntry->Attribute("content");
		wstr = UTF8ToWide(str);
		offset = WriteWideStringData(wstr);
		//
		updata.pos = curSize;
		updata.offset = offset - curSize + 4;
		v_updateLE_string.push_back(updata);
		curSize += 4;

		// end
		infoCount++;
	}
	*pSize = infoCount;

	// check alignment
	if (infoCount % 2) {
		infoCount++;
	}

	std::vector<char> out(infoCount * 8, 0);
	return out;
}

std::vector<char> RMPA6::WriteCamera(tinyxml2::XMLElement* xmlData, int inSize)
{
	// initialization
	std::vector<char> buffer(std::begin(RAW_MainNode), std::end(RAW_MainNode));
	DataNodeCount++;

	updateDataOffset_t out_dataup;
	out_dataup.pos = inSize + 8;
	out_dataup.offset = WriteCommonWideString(xmlData, "name", &buffer[4], -inSize);
	v_update_string.push_back(out_dataup);

	return buffer;
}

std::vector<char> RMPA6::WritePoint(tinyxml2::XMLElement* xmlData, int inSize)
{
	// initialization
	std::vector<char> buffer(std::begin(RAW_MainNode), std::end(RAW_MainNode));
	DataNodeCount++;

	updateDataOffset_t out_dataup;
	out_dataup.pos = inSize + 8;
	out_dataup.offset = WriteCommonWideString(xmlData, "name", &buffer[4], -inSize);
	v_update_string.push_back(out_dataup);

	// set temp
	std::vector<char> v_temp;
	std::vector<updateDataOffset_t> update_data;
	tinyxml2::XMLElement* entry;

	// read node
	std::vector<char> v_node;
	std::vector<tinyxml2::XMLElement*> v_xmlNode;
	int i_nodeCount = 0;
	for (entry = xmlData->FirstChildElement("Node"); entry != 0; entry = entry->NextSiblingElement("Node")) {
		// set initial
		v_temp.resize(0x14);
		WriteINT32LE(&v_temp[0], -1);
		WriteINT32LE(&v_temp[4], 0);
		// get name
		int nodePos = inSize + v_node.size() + 0x20;
		out_dataup.pos = nodePos + 8;
		out_dataup.offset = WriteCommonWideString(entry, "name", &v_temp[4], -nodePos);
		v_update_string.push_back(out_dataup);
		// set data info pos
		out_dataup.pos = v_node.size();
		out_dataup.offset = i_nodeCount;
		update_data.push_back(out_dataup);

		// end
		v_xmlNode.push_back(entry);
		v_node.insert(v_node.end(), v_temp.begin(), v_temp.end());
		v_temp.clear();
		i_nodeCount++;
	}
	WriteBEdword(&buffer[0xC], i_nodeCount);
	WriteBEdword(&buffer[0x10], 0x20);
	// check 16-byte alignment
	int align = v_node.size() % 16;
	if (align) {
		for (int i = align; i < 16; i++) {
			v_node.push_back(0);
		}
	}

	int bufferSize = v_node.size() + 0x20 + inSize;
	// read data
	std::vector<char> v_data;
	BEtoLEByte_t dataNodeCount;
	for (int i = 0; i < v_xmlNode.size(); i++) {
		v_temp = WritePointData(v_xmlNode[i], bufferSize, &dataNodeCount.s32);
		int curpos = update_data[i].pos;
		WriteBEdword(&v_node[curpos + 0xC], dataNodeCount.u32);

		// here dataNodeCount as offset
		dataNodeCount.s32 = v_node.size() - curpos + v_data.size();
		WriteBEdword(&v_node[curpos + 0x10], dataNodeCount.u32);

		// write to buffer
		bufferSize += v_temp.size();
		v_data.insert(v_data.end(), v_temp.begin(), v_temp.end());
		v_temp.clear();
	}
	update_data.clear();
	v_xmlNode.clear();

	// merge data
	WriteBEdword(&buffer[0x10], 0x20);
	buffer.insert(buffer.end(), v_node.begin(), v_node.end());
	v_node.clear();
	//
	buffer.insert(buffer.end(), v_data.begin(), v_data.end());
	v_data.clear();

	return buffer;
}

std::vector<char> RMPA6::WritePointData(tinyxml2::XMLElement* xmlData, int baseSize, int* nodeCount)
{
	tinyxml2::XMLElement* entry;
	std::vector<tinyxml2::XMLElement*> v_xmlNode;
	outPointNode_t out_node;
	std::vector<outPointNode_t> v_out_node;

	// preprocess
	int dataCount = 0;
	for (entry = xmlData->FirstChildElement("Data"); entry != 0; entry = entry->NextSiblingElement("Data")) {
		v_xmlNode.push_back(entry);
		DataNodeCount++;

		out_node.in.IndexID = DataNodeCount;
		out_node.pos = dataCount * 0x34;
		v_out_node.push_back(out_node);

		dataCount++;
	}
	*nodeCount = dataCount;
	int dataSize = dataCount * 0x34;
	std::vector<char> out(dataSize);
	// check 16-byte alignment
	int align = dataSize % 16;
	if (align) {
		for (int i = align; i < 16; i++) {
			out.push_back(0);
		}
		dataSize = out.size();
	}

	// get data
	updateDataOffset_t out_dataup;
	for (int i = 0; i < v_xmlNode.size(); i++) {
		auto pNode = &v_out_node[i];
		entry = v_xmlNode[i];

		// get name
		int nodePos = baseSize + pNode->pos;
		out_dataup.pos = nodePos + 0x28;
		out_dataup.offset = WriteCommonWideString(entry, "name", &out[pNode->pos + 0x24], -nodePos);
		v_update_string.push_back(out_dataup);

		// get data
		auto xmlNode = entry->FirstChildElement("position");
		WriteFloat4FromXML(xmlNode, pNode->in.pos1);
		xmlNode = entry->FirstChildElement("orientation");
		WriteFloat4FromXML(xmlNode, pNode->in.pos2);

		// update data
		WriteBEdwordGroup(&out[pNode->pos], (UINT32*)&pNode->in.IndexID, 9);
		WriteINT32LE(&out[pNode->pos + 0x2C], 0);
		WriteINT32LE(&out[pNode->pos + 0x30], 0);
	}

	return out;
}
