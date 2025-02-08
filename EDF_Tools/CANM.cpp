#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <algorithm>

#include "util.h"
#include "CANM.h"
#include "include/tinyxml2.h"
#include "include/half.hpp"

void CANM::Read(const std::wstring& path)
{
	std::ifstream file(path + L".canm", std::ios::binary | std::ios::ate | std::ios::in);

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	if (file.read(buffer.data(), size))
	{
		// create xml
		tinyxml2::XMLDocument xml;
		xml.InsertFirstChild(xml.NewDeclaration());
		tinyxml2::XMLElement* xmlHeader = xml.NewElement("CANM");
		xml.InsertEndChild(xmlHeader);

		ReadData(buffer, xmlHeader);

		std::string outfile = WideToUTF8(path) + "_CANM.xml";
		xml.SaveFile(outfile.c_str());
		/*
		tinyxml2::XMLPrinter printer;
		xml.Accept(&printer);
		auto xmlString = std::string{ printer.CStr() };

		std::wcout << UTF8ToWide(xmlString) + L"\n";
		*/
	}

	//Clear buffers
	buffer.clear();
	file.close();
}

void CANM::ReadData(const std::vector<char>& buffer, tinyxml2::XMLElement* header)
{
	int position = 0;
	unsigned char seg[4];
	// read header, length is 0x20
	memcpy(&seg, &buffer[0], 4U);
	//if (seg[0] == 0x43 && seg[1] == 0x41 && seg[2] == 0x4E && seg[3] == 0x4D)	

	// Check version
	GameVersion = ReadInt32(&buffer[0x4], 0);
	if (GameVersion == 768) {
		header->SetAttribute("version", "6");
	}
	// read AnmData
	i_AnmDataCount = ReadInt32(&buffer[0x8], 0);
	i_AnmDataOffset = ReadInt32(&buffer[0xC], 0);
	// read AnmPoint
	i_AnmPointCount = ReadInt32(&buffer[0x10], 0);
	i_AnmPointOffset = ReadInt32(&buffer[0x14], 0);
	// read bone
	i_BoneCount = ReadInt32(&buffer[0x18], 0);
	i_BoneOffset = ReadInt32(&buffer[0x1C], 0);

	// bone data
	std::wcout << L"Read CANM......\n";
	std::wcout << L"Read bone list...... ";
	ReadBoneListData(header, buffer);
	std::wcout << L"Complete!\n";
	// animation key data
	//ReadAnimationPointData(header, buffer);
	std::wcout << L"Read animation frame list:\n0     in " + ToString(i_AnmPointCount);
	std::vector<char> tempBuffer = buffer;
	v_AnmKey.reserve(i_AnmPointCount);
	if (GameVersion == 768) {
		// 0x30 if it is EDF6
		for (int i = 0; i < i_AnmPointCount; i++) {
			int curpos = i_AnmPointOffset + (i * 0x30);

			v_AnmKey.push_back(ReadAnimationFrameData6(tempBuffer, curpos));

			std::wcout << L"\r" + ToString(i + 1);
		}
	}
	else {
		for (int i = 0; i < i_AnmPointCount; i++)
		{
			int curpos = i_AnmPointOffset + (i * 0x20);

			//v_AnmKey.push_back(ReadAnimationFrameData(buffer, curpos));
			v_AnmKey.push_back(ReadAnimationFrameData(tempBuffer, curpos));

			std::wcout << L"\r" + ToString(i+1);
			// now not displayed as a percentage
			//float Progress = i * 100.0f / i_AnmPointCount;
			//std::wcout << L"\r" + ToString(Progress) + L"\%";
		}
	}
	std::wcout << L"\nComplete!\n";
	// animation data
	std::wcout << L"Read animation list:\n0     in " + ToString(i_AnmDataCount);
	ReadAnimationData(header, buffer);
	std::wcout << L"\nComplete!\n";
	std::wcout << L"===>CANM parsing completed!\n";
}

void CANM::ReadAnimationData(tinyxml2::XMLElement* header, const std::vector<char>& buffer)
{
	tinyxml2::XMLElement* xmldata = header->InsertNewChildElement("AnmData");
	for (int i = 0; i < i_AnmDataCount; i++)
	{
		int curpos = i_AnmDataOffset + (i * 0x1C);

		int value[7];
		memcpy(&value, &buffer[curpos], 28U);
		tinyxml2::XMLElement* xmlptr = xmldata->InsertNewChildElement("node");

		xmlptr->SetAttribute("index", i);
#if defined(DEBUGMODE)
		xmlptr->SetAttribute("pos", curpos);
		xmlptr->SetAttribute("boneCount", value[5]);
#endif

		// i0 is int.
		xmlptr->SetAttribute("int1", value[0]);

		// get string
		std::wstring wstr;
		// str
		if (value[1] > 0)
			wstr = ReadUnicode(buffer, curpos + value[1]);
		else
			wstr = L"";
		std::string utf8str = WideToUTF8(wstr);
		xmlptr->SetAttribute("name", utf8str.c_str());

		// i2 is float
		xmlptr->SetAttribute("time", IntHexAsFloat(value[2]));
		// i3 is float
		xmlptr->SetAttribute("speed", IntHexAsFloat(value[3]));
		// i4 is int.
		xmlptr->SetAttribute("kf", value[4]);

		// i5 is amount, i6 is offset
		for (int j = 0; j < value[5]; j++)
		{
			int datapos = curpos + value[6] + (j * 8);

			short number[4];
			memcpy(&number, &buffer[datapos], 8U);

			tinyxml2::XMLElement* xmlNode = xmlptr->InsertNewChildElement("value");

#if defined(DEBUGMODE)
			xmlNode->SetAttribute("pos", datapos);
#endif

			xmlNode->SetAttribute("bone", BoneList[number[0]].c_str());
			//xmlNode->SetAttribute("kpos", number[1]);
			//xmlNode->SetAttribute("krot", number[2]);
			//xmlNode->SetAttribute("ktra", number[3]);

			tinyxml2::XMLElement* xmlPos = xmlNode->InsertNewChildElement("position");
			if (number[1] < 0)
				xmlPos->SetAttribute("type", "null");
			else
				ReadAnimationDataWriteKeyFrame(xmlPos, number[1]);

			tinyxml2::XMLElement* xmlRot = xmlNode->InsertNewChildElement("rotation");
			if (number[2] < 0)
				xmlRot->SetAttribute("type", "null");
			else
				ReadAnimationDataWriteKeyFrame(xmlRot, number[2]);

			tinyxml2::XMLElement* xmlVis = xmlNode->InsertNewChildElement("scaling");
			if (number[3] < 0)
				xmlVis->SetAttribute("type", "null");
			else
				ReadAnimationDataWriteKeyFrame(xmlVis, number[3]);
		}
		std::wcout << L"\r" + ToString(i + 1);
	}
	// end
}

void CANM::ReadAnimationDataWriteKeyFrame(tinyxml2::XMLElement* node, int num)
{
#if defined(DEBUGMODE)
	node->SetAttribute("pos", v_AnmKey[num].pos);
#endif

	node->SetAttribute("type", v_AnmKey[num].vi[0]);
	node->SetAttribute("frame", v_AnmKey[num].vi[1]);
	node->SetAttribute("ix", v_AnmKey[num].vf[0]);
	node->SetAttribute("iy", v_AnmKey[num].vf[1]);
	node->SetAttribute("iz", v_AnmKey[num].vf[2]);
	node->SetAttribute("vx", v_AnmKey[num].vf[3]);
	node->SetAttribute("vy", v_AnmKey[num].vf[4]);
	node->SetAttribute("vz", v_AnmKey[num].vf[5]);
	// write keyframe to xml
	if (v_AnmKey[num].kf.size() > 0)
	{
		for (int i = 0; i < v_AnmKey[num].kf.size(); i++)
		{
			tinyxml2::XMLElement* xmlNode = node->InsertNewChildElement("v");

			xmlNode->SetAttribute("x", v_AnmKey[num].kf[i].vf[0]);
			xmlNode->SetAttribute("y", v_AnmKey[num].kf[i].vf[1]);
			xmlNode->SetAttribute("z", v_AnmKey[num].kf[i].vf[2]);
			// debug mode output half
#if defined(DEBUGMODE)
			//xmlNode->SetAttribute("pos", v_AnmKey[num].pos);
			half_float::half vf[3];
			memcpy(&vf, &v_AnmKey[num].kf[i].vf, 6U);

			xmlNode->SetAttribute("dx", vf[0]);
			xmlNode->SetAttribute("dy", vf[1]);
			xmlNode->SetAttribute("dz", vf[2]);
#endif
		}
	}
}

void CANM::ReadAnimationPointData(tinyxml2::XMLElement* header, const std::vector<char>& buffer)
{
	tinyxml2::XMLElement* xmldata = header->InsertNewChildElement("AnmKey");
	for (int i = 0; i < i_AnmPointCount; i++)
	{
		int curpos = i_AnmPointOffset + (i * 0x20);

		short value[2];
		memcpy(&value, &buffer[curpos], 4U);
		tinyxml2::XMLElement* xmlptr = xmldata->InsertNewChildElement("node");

		xmlptr->SetAttribute("index", i);
#if defined(DEBUGMODE)
		xmlptr->SetAttribute("pos", curpos);
#endif

		xmlptr->SetAttribute("type", value[0]);
		xmlptr->SetAttribute("frame", value[1]);

		// get float
		float vf[6];
		memcpy(&vf, &buffer[curpos + 4], 24U);
		xmlptr->SetAttribute("ix", vf[0]);
		xmlptr->SetAttribute("iy", vf[1]);
		xmlptr->SetAttribute("iz", vf[2]);
		xmlptr->SetAttribute("vx", vf[3]);
		xmlptr->SetAttribute("vy", vf[4]);
		xmlptr->SetAttribute("vz", vf[5]);

		// keyframe offset
		int offset;
		memcpy(&offset, &buffer[curpos + 28], 4U);
		if (offset > 0)
		{
			for (int j = 0; j < value[1]; j++)
			{
				int datapos = curpos + offset + (j * 6);

				tinyxml2::XMLElement* xmlNode = xmlptr->InsertNewChildElement("v");

				// debug mode output int16
#if defined(DEBUGMODE)
				xmlNode->SetAttribute("pos", datapos);

				short vf[3];
				memcpy(&vf, &buffer[datapos], 6U);

				xmlNode->SetAttribute("x", vf[0]);
				xmlNode->SetAttribute("y", vf[1]);
				xmlNode->SetAttribute("z", vf[2]);
#else
				half_float::half vf[3];
				memcpy(&vf, &buffer[datapos], 6U);

				xmlNode->SetAttribute("x", vf[0]);
				xmlNode->SetAttribute("y", vf[1]);
				xmlNode->SetAttribute("z", vf[2]);
#endif
				// end
			}
		}
	}
	// end
}

void CANM::ReadBoneListData(tinyxml2::XMLElement* header, const std::vector<char>& buffer)
{
	tinyxml2::XMLElement* xmlbone = header->InsertNewChildElement("BoneList");
	for (int i = 0; i < i_BoneCount; i++)
	{
		int curpos = i_BoneOffset + (i * 4);

		int boneofs;
		memcpy(&boneofs, &buffer[curpos], 4U);
		tinyxml2::XMLElement* xmlptr = xmlbone->InsertNewChildElement("value");

#if defined(DEBUGMODE)
		xmlptr->SetAttribute("index", i);
		xmlptr->SetAttribute("pos", curpos);
#endif

		// get string
		std::wstring wstr;
		// str
		if (boneofs > 0)
			wstr = ReadUnicode(buffer, curpos + boneofs);
		else
			wstr = L"";
		std::string utf8str = WideToUTF8(wstr);
		xmlptr->SetText(utf8str.c_str());
		BoneList.push_back(utf8str);
	}
}

CANMAnmKey CANM::ReadAnimationFrameData(const std::vector<char>& buffer, int pos)
{
	CANMAnmKey out;

	memcpy(&out.vi, &buffer[pos], 4U);
	memcpy(&out.vf, &buffer[pos + 4], 24U);
	// read keyframe offset
	int offset;
	memcpy(&offset, &buffer[pos + 28], 4U);
	if (offset > 0)
	{
		for (int j = 0; j < out.vi[1]; j++)
		{
			int datapos = pos + offset + (j * 6);

			CANMAnmKeyframe kfout;
			memcpy(&kfout.vf, &buffer[datapos], 6U);
			out.kf.push_back(kfout);
		}
	}
	// only debug
	out.pos = pos;
	return out;
}

CANMAnmKey CANM::ReadAnimationFrameData6(const std::vector<char>& buffer, int pos)
{
	CANMAnmKey out;

	float vf[4];
	memcpy(&vf, &buffer[pos], 0x10);
	memcpy(&out.vf[0], &vf, 12U);
	// There are 2 groups. vf3 is 1.0f
	memcpy(&vf, &buffer[pos+0x10], 0x10);
	memcpy(&out.vf[3], &vf, 12U);

	// read keyframe offset
	int offset = ReadInt32(&buffer[pos+0x20], 0);
	// todo: verify that it is 0-3, and function.
	int index = ReadInt32(&buffer[pos + 0x24], 0);
	int kfSize = ReadInt32(&buffer[pos + 0x28], 0);
	out.vi[1] = kfSize;
	int loop = ReadInt32(&buffer[pos + 0x2C], 0);
	out.vi[0] = loop;
	if (offset > 0)
	{
		for (int j = 0; j < out.vi[1]; j++)
		{
			int datapos = pos + offset + (j * 6);

			CANMAnmKeyframe kfout;
			memcpy(&kfout.vf, &buffer[datapos], 6U);
			out.kf.push_back(kfout);
		}
	}
	// only debug
	out.pos = pos;
	return out;
}

void CANM::Write(const std::wstring& path)
{
	std::wstring sourcePath = path + L"_canm.xml";
	std::wcout << "Will output CANM file.\n";
	std::string UTF8Path = WideToUTF8(sourcePath);

	tinyxml2::XMLDocument doc;
	doc.LoadFile(UTF8Path.c_str());

	tinyxml2::XMLElement* header = doc.FirstChildElement("CANM");

	std::vector< char > bytes;
	bytes = WriteData(header);

	//Final write.
	/**/
	std::ofstream newFile(path + L".CANM", std::ios::binary | std::ios::out | std::ios::ate);

	newFile.write(bytes.data(), bytes.size());

	newFile.close();
	
	std::wcout << L"Conversion completed: " + path + L".canm\n";
}

std::vector<char> CANM::WriteData(tinyxml2::XMLElement* Data)
{
	std::vector< char > bytes, kfbytes, bdbytes;
	tinyxml2::XMLElement* entry, * entry2;
	
	// read bone name table (if there is)
	std::string bstr;
	entry = Data->FirstChildElement("BoneList");
	if (entry != nullptr)
	{
		for (entry2 = entry->FirstChildElement("value"); entry2 != 0; entry2 = entry2->NextSiblingElement("value"))
		{
			bstr = entry2->GetText();
			WBoneList.push_back(UTF8ToWide(bstr));
			// No need to increase i_BoneCount
			// Because finally we get the size of WBoneList
		}
	}

	// read animation point, not using it now
	/*
	entry = Data->FirstChildElement("AnmKey");
	for (entry2 = entry->FirstChildElement("node"); entry2 != 0; entry2 = entry2->NextSiblingElement("node"))
	{
		// keyframes are written at the same time
		v_AnmPoint.push_back(WriteAnmPoint(entry2, &kfbytes));
	}
	// write keyframe offset
	i_AnmPointCount = v_AnmPoint.size();
	int size_AnmPoint = i_AnmPointCount * 0x20;
	for (int i = 0; i < i_AnmPointCount; i++)
	{
		if (v_AnmPoint[i].haskey)
		{
			int offset = v_AnmPoint[i].offset + size_AnmPoint - v_AnmPoint[i].pos;
			memcpy(&v_AnmPoint[i].bytes[0x1C], &offset, 4U);
		}
	}
	*/

	// read animation data
	std::wcout << L"Read CANM animation list:\n0";
	entry = Data->FirstChildElement("AnmData");
	for (entry2 = entry->FirstChildElement("node"); entry2 != 0; entry2 = entry2->NextSiblingElement("node"))
	{
		// bone data is written at the same time
		//v_AnmData.push_back(WriteAnmData(entry2, &bdbytes));
		v_AnmData.push_back(WriteAnimationData(entry2, &bdbytes));
		std::wcout << L"\r" + std::to_wstring(v_AnmData.size());
	}
	std::wcout << L" ===> Complete!\n";
	// write bone data offset
	i_AnmDataCount = v_AnmData.size();
	int size_AnmData = i_AnmDataCount * 0x1C;
	for (int i = 0; i < i_AnmDataCount; i++)
	{
		int offset = v_AnmData[i].offset + size_AnmData - v_AnmData[i].pos;
		memcpy(&v_AnmData[i].bytes[0x18], &offset, 4U);
	}
	// write keyframe offset
	std::wcout << L"Read CANM animation keyframe:\n0";
	i_AnmPointCount = v_AnmKey.size();
	int size_AnmPoint = i_AnmPointCount * 0x20;
	for (int i = 0; i < i_AnmPointCount; i++)
	{
		if (v_AnmKey[i].kf.size() == 1)
		{
			int offset = kfbytes.size() + size_AnmPoint - v_AnmKey[i].pos;
			memcpy(&v_AnmKey[i].bytes[0x1C], &offset, 4U);
			kfbytes.insert(kfbytes.end(), v_AnmKey[i].kf[0].bytes.begin(), v_AnmKey[i].kf[0].bytes.end());
		}
		std::wcout << L"\r" + std::to_wstring(i + 1);
	}
	std::wcout << L" ===> Complete!\n";

	// write bone list
	i_BoneCount = WBoneList.size();
	std::vector< char > blbytes = WriteBoneList(i_BoneCount);

	// generate header
	bytes.resize(0x20, 0);
	bytes[0] = 0x43;
	bytes[1] = 0x41;
	bytes[2] = 0x4E;
	bytes[3] = 0x4D;
	bytes[5] = 0x02;

	// write animation point data
	std::wcout << L"Write CANM animation keyframe......";
	for (size_t i = 0; i < v_AnmKey.size(); i++)
	{
		bytes.insert(bytes.end(), v_AnmKey[i].bytes.begin(), v_AnmKey[i].bytes.end());
		//std::wcout << L"\r" + std::to_wstring(i + 1);
	}
	//std::wcout << L" Complete!\n";
	// write keyframe data
	//std::wcout << L"Write CANM animation keyframe data......";
	//float progress = kfbytes.size() / 100.0f;
	bytes.insert(bytes.end(), kfbytes.begin(), kfbytes.end());
	std::wcout << L" Complete!\n";
	// write animation point header
	bytes[0x14] = 0x20;
	memcpy(&bytes[0x10], &i_AnmPointCount, 4U);
	// 4-byte alignment is required
	int i_Alignment = bytes.size() % 4;
	if (i_Alignment > 0)
	{
		for (int i = i_Alignment; i < 4; i++)
			bytes.push_back(0);
	}

	i_AnmDataOffset = bytes.size();
	// write animation data
	std::wcout << L"Write CANM animation list......";
	for (size_t i = 0; i < v_AnmData.size(); i++)
	{
		// need to save this location
		v_AnmData[i].pos = bytes.size();
		bytes.insert(bytes.end(), v_AnmData[i].bytes.begin(), v_AnmData[i].bytes.end());
		//std::wcout << L"\r" + std::to_wstring(i + 1);
	}
	std::wcout << L" Complete!\n";
	// write bone data
	bytes.insert(bytes.end(), bdbytes.begin(), bdbytes.end());
	// write animation data header
	memcpy(&bytes[0x8], &i_AnmDataCount, 4U);
	memcpy(&bytes[0xC], &i_AnmDataOffset, 4U);

	// write bone list
	i_BoneOffset = bytes.size();
	bytes.insert(bytes.end(), blbytes.begin(), blbytes.end());
	// write bone list header
	memcpy(&bytes[0x18], &i_BoneCount, 4U);
	memcpy(&bytes[0x1C], &i_BoneOffset, 4U);

	// write remaining string
	for (size_t i = 0; i < v_AnmData.size(); i++)
	{
		int curpos = bytes.size() - v_AnmData[i].pos;
		memcpy(&bytes[v_AnmData[i].pos + 4], &curpos, 4U);
		PushWStringToVector(v_AnmData[i].wstr, &bytes);
	}

	return bytes;
}

CANMAnmPoint CANM::WriteAnmPoint(tinyxml2::XMLElement* data, std::vector< char >* bytes)
{
	CANMAnmPoint out;

	float fvalue[6];
	fvalue[0] = data->FloatAttribute("ix");
	fvalue[1] = data->FloatAttribute("iy");
	fvalue[2] = data->FloatAttribute("iz");
	fvalue[3] = data->FloatAttribute("vx");
	fvalue[4] = data->FloatAttribute("vy");
	fvalue[5] = data->FloatAttribute("vz");

	short svalue[2];
	svalue[0] = 0;
	svalue[1] = 1;

	out.pos = v_AnmPoint.size() * 0x20;
	out.offset = bytes->size();
	out.haskey = false;
	// write keyframe
	tinyxml2::XMLElement* entry = data->FirstChildElement("v");
	if (entry != nullptr)
	{
		half_float::half hf[3];
		char buffer[6];
		short count = 0;

		for (entry = data->FirstChildElement("v"); entry != 0; entry = entry->NextSiblingElement("v"))
		{
			// debug mode input int16
#if defined(DEBUGMODE)
			short vi[3];
			vi[0] = entry->IntAttribute("x");
			vi[1] = entry->IntAttribute("y");
			vi[2] = entry->IntAttribute("z");
			memcpy(&buffer, &vi, 6U);
#else
			hf[0] = entry->FloatAttribute("x");
			hf[1] = entry->FloatAttribute("y");
			hf[2] = entry->FloatAttribute("z");
			memcpy(&buffer, &hf, 6U);
#endif
			// end
			count++;
			for (int i = 0; i < 6; i++)
				bytes->push_back(buffer[i]);
		}

		svalue[0] = 1;
		svalue[1] = count;
		out.haskey = true;
	}

	out.bytes.resize(0x20, 0);
	memcpy(&out.bytes[0], &svalue, 4U);
	memcpy(&out.bytes[4], &fvalue, 24U);

	return out;
}

CANMAnmData CANM::WriteAnmData(tinyxml2::XMLElement* data, std::vector<char>* bytes)
{
	CANMAnmData out;
	out.bytes.resize(0x1C, 0);

	out.pos = v_AnmData.size() * 0x1C;
	out.offset = bytes->size();
	out.wstr = UTF8ToWide(data->Attribute("name"));
	// - 0x00 - : Int32
	int v1 = data->IntAttribute("int1");
	memcpy(&out.bytes[0], &v1, 4U);
	// - 0x08 - : Float, animation time.
	// - 0x0C - : Float. Value = 0x08's value / (0x10's value - 1). 
	float vf[2];
	vf[0] = data->FloatAttribute("time");
	vf[1] = data->FloatAttribute("speed");
	memcpy(&out.bytes[0x8], &vf, 8U);
	// - 0x10 - : Int32, multiplier, but add 1.
	int v2 = data->IntAttribute("kf");
	memcpy(&out.bytes[0x10], &v2, 4U);
	// write bone data
	int count = 0;
	tinyxml2::XMLElement* entry = data->FirstChildElement("value");
	if (entry != nullptr)
	{
		short svalue[4];
		std::wstring wstr;
		char buffer[8];
		for (entry = data->FirstChildElement("value"); entry != 0; entry = entry->NextSiblingElement("value"))
		{
			wstr = UTF8ToWide(entry->Attribute("bone"));
			// check for duplication
			bool isExist = false;

			for (size_t i = 0; i < WBoneList.size(); i++)
			{
				if (WBoneList[i] == wstr)
				{
					isExist = true;
					svalue[0] = i;

					break;
				}
			}

			if (!isExist)
			{
				svalue[0] = WBoneList.size();
				WBoneList.push_back(wstr);
			}

			// read other
			svalue[1] = entry->IntAttribute("kpos");
			svalue[2] = entry->IntAttribute("krot");
			svalue[3] = entry->IntAttribute("ktra");
			count++;
			// write data
			memcpy(&buffer, &svalue, 8U);
			for (int i = 0; i < 8; i++)
				bytes->push_back(buffer[i]);
		}
	}
	memcpy(&out.bytes[0x14], &count, 4U);

	return out;
}

std::vector<char> CANM::WriteBoneList(int in)
{
	std::vector<char> out(in * 4, 0);
	// get string data
	for (size_t i = 0; i < WBoneList.size(); i++)
	{
		int pos = i * 4;
		int offset = out.size() - pos;
		memcpy(&out[pos], &offset, 4U);
		// write string
		PushWStringToVector(WBoneList[i], &out);
	}

	return out;
}

CANMAnmData CANM::WriteAnimationData(tinyxml2::XMLElement* data, std::vector<char>* bytes)
{
	CANMAnmData out;
	out.bytes.resize(0x1C, 0);

	out.pos = v_AnmData.size() * 0x1C;
	out.offset = bytes->size();
	out.wstr = UTF8ToWide(data->Attribute("name"));
	// - 0x00 - : Int32
	int v1 = data->IntAttribute("int1");
	memcpy(&out.bytes[0], &v1, 4U);
	// - 0x08 - : Float, animation time.
	// - 0x0C - : Float. Value = 0x08's value / (0x10's value - 1). 
	float vf[2];
	vf[0] = data->FloatAttribute("time");
	vf[1] = data->FloatAttribute("speed");
	memcpy(&out.bytes[0x8], &vf, 8U);
	// - 0x10 - : Int32, multiplier, but add 1.
	int v2 = data->IntAttribute("kf");
	memcpy(&out.bytes[0x10], &v2, 4U);
	// write bone data
	int count = 0;
	tinyxml2::XMLElement* entry = data->FirstChildElement("value");
	if (entry != nullptr)
	{
		short svalue[4];
		std::wstring wstr;
		char buffer[8];
		for (entry = data->FirstChildElement("value"); entry != 0; entry = entry->NextSiblingElement("value"))
		{
			wstr = UTF8ToWide(entry->Attribute("bone"));
			// check for duplication
			bool isExist = false;

			for (size_t i = 0; i < WBoneList.size(); i++)
			{
				if (WBoneList[i] == wstr)
				{
					isExist = true;
					svalue[0] = i;

					break;
				}
			}

			if (!isExist)
			{
				svalue[0] = WBoneList.size();
				WBoneList.push_back(wstr);
			}

			// read other
			svalue[1] = WriteAnimationKeyFrame(entry->FirstChildElement("position"));
			svalue[2] = WriteAnimationKeyFrame(entry->FirstChildElement("rotation"));
			svalue[3] = WriteAnimationKeyFrame(entry->FirstChildElement("scaling"));
			count++;
			// write data
			memcpy(&buffer, &svalue, 8U);
			for (int i = 0; i < 8; i++)
				bytes->push_back(buffer[i]);
		}
	}
	memcpy(&out.bytes[0x14], &count, 4U);

	return out;
}

short CANM::WriteAnimationKeyFrame(tinyxml2::XMLElement* data)
{
	std::string type = data->Attribute("type");
	if (type == "null")
	{
		return -1;
	}
	else
	{
		CANMAnmKey out;

		float fvalue[6];
		fvalue[0] = data->FloatAttribute("ix");
		fvalue[1] = data->FloatAttribute("iy");
		fvalue[2] = data->FloatAttribute("iz");
		fvalue[3] = data->FloatAttribute("vx");
		fvalue[4] = data->FloatAttribute("vy");
		fvalue[5] = data->FloatAttribute("vz");

		short svalue[2];
		svalue[0] = 0;
		svalue[1] = 1;

		out.pos = v_AnmKey.size() * 0x20;
		out.offset = 0;
		// write keyframe
		tinyxml2::XMLElement* entry = data->FirstChildElement("v");
		if (entry != nullptr)
		{
			UINT16 vi[3];
			char buffer[6];
			short count = 0;
			CANMAnmKeyframe kfout;

			for (entry = data->FirstChildElement("v"); entry != 0; entry = entry->NextSiblingElement("v"))
			{
				// now input int16
				vi[0] = entry->IntAttribute("x");
				vi[1] = entry->IntAttribute("y");
				vi[2] = entry->IntAttribute("z");
				memcpy(&buffer, &vi, 6U);
				// end
				count++;
				for (int i = 0; i < 6; i++)
					kfout.bytes.push_back(buffer[i]);
			}

			svalue[0] = 1;
			svalue[1] = count;
			out.kf.push_back(kfout);
		}

		out.bytes.resize(0x20, 0);
		memcpy(&out.bytes[0], &svalue, 4U);
		memcpy(&out.bytes[4], &fvalue, 24U);

		// check for duplication
		bool isExist = false;
		for (size_t i = 0; i < v_AnmKey.size(); i++)
		{
			if (out.bytes == v_AnmKey[i].bytes)
			{
				// check subobject, if it exists
				if (svalue[0] == 1)
				{
					if (out.kf[0].bytes == v_AnmKey[i].kf[0].bytes)
					{
						isExist = true;
						return i;
					}
					else
					{
						break;
					}
				}
				else
				{
					isExist = true;
					return i;
				}
			}
		}

		if (!isExist)
		{
			short index = v_AnmKey.size();
			v_AnmKey.push_back(out);
			return index;
		}
	}
}
