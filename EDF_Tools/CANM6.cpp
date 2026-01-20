#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <algorithm>

#include "util.h"
#include "CANM6.h"

//#define DEBUGMODE

void CANM6::ReadData(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader)
{
	// get header
	memcpy(&header, &buffer[0], 0x20);
	std::wcout << L"Read CANM......\n";
	// get bone name list
	std::wcout << L"Read bone list...... ";
	ReadBoneListData(buffer, xmlHeader);
	std::wcout << L"Complete!\n";
	// get keyframe data
	ReadKeyframeData(buffer, xmlHeader);
	// get animation data
	ReadAnimationData(buffer, xmlHeader);
	std::wcout << L"===>CANM parsing completed!\n";
}

void CANM6::ReadAnimationData(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader)
{
	inAnimationData_t Animation;
	inBoneData_t boneData;
	int inputDataCount = header.AnmDataCount;

	tinyxml2::XMLElement* xmldata = xmlHeader->InsertNewChildElement("AnmData");
	tinyxml2::XMLElement* xmlptr;
	tinyxml2::XMLElement* xmlNode;
	std::wstring wstr;
	std::string utf8str;

	std::wcout << L"Read animation list:\n0     in " + ToString(inputDataCount);
	for (int i = 0; i < inputDataCount; i++) {
		int curpos = header.AnmDataOffset + (i * 0x1C);
		memcpy(&Animation, &buffer[curpos], 0x1C);

		xmlptr = xmldata->InsertNewChildElement("node");
		xmlptr->SetAttribute("index", i);

#if defined(DEBUGMODE)
		xmlptr->SetAttribute("pos", curpos);
#endif

		// get is loop?
		xmlptr->SetAttribute("loop", Animation.pad0);

		// get string
		if (Animation.nameOffset > 0)
			wstr = ReadUnicode(buffer, curpos + Animation.nameOffset);
		else
			wstr = L"";
		utf8str = WideToUTF8(wstr);
		xmlptr->SetAttribute("name", utf8str.c_str());

		xmlptr->SetAttribute("time", Animation.time);
		xmlptr->SetAttribute("speed", Animation.speed);
		xmlptr->SetAttribute("keyframe", Animation.keyframe);

		// get bone data
		int datapos = curpos + Animation.BoneOffset;
		for (int j = 0; j < Animation.BoneCount; j++) {
			memcpy(&boneData, &buffer[datapos], 8U);

			xmlNode = xmlptr->InsertNewChildElement("value");

#if defined(DEBUGMODE)
			xmlNode->SetAttribute("pos", datapos);
#endif

			xmlNode->SetAttribute("bone", BoneList[boneData.bone].c_str());

			tinyxml2::XMLElement* xmlPos = xmlNode->InsertNewChildElement("position");
			if (boneData.position < 0)
				xmlPos->SetAttribute("type", "null");
			else
				ReadAnimationDataWriteKeyFrame(buffer, xmlPos, boneData.position);

			tinyxml2::XMLElement* xmlRot = xmlNode->InsertNewChildElement("rotation");
			if (boneData.rotation < 0)
				xmlRot->SetAttribute("type", "null");
			else
				ReadAnimationDataWriteKeyFrame(buffer, xmlRot, boneData.rotation);

			tinyxml2::XMLElement* xmlVis = xmlNode->InsertNewChildElement("scaling");
			if (boneData.scaling < 0)
				xmlVis->SetAttribute("type", "null");
			else
				ReadAnimationDataWriteKeyFrame(buffer, xmlVis, boneData.scaling);

			datapos += 8;
		}

		// refresh log
		std::wcout << L"\r" + ToString(i + 1);
	}
	std::wcout << L"\nComplete!\n";
}

void CANM6::ReadAnimationDataWriteKeyFrame(const std::vector<char>& buffer, tinyxml2::XMLElement* node, int num)
{
	if (num > v_KeyframeData.size()) {
		node->SetAttribute("type", "null");
		return;
	}

	canmKeyframeData_t & Keyframe = v_KeyframeData[num];
	return ReadKeyframeDataText(buffer, node, Keyframe);
}

void CANM6::ReadKeyframeDataText(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlNode, const canmKeyframeData_t& Keyframe)
{
	int KFCount = Keyframe.bin.keyframe;
	tinyxml2::XMLElement* xmlKF;

	xmlNode->SetAttribute("type", Keyframe.bin.type);
	xmlNode->SetAttribute("frame", KFCount);
	if (Keyframe.bin.pad2C) {
		xmlNode->SetAttribute("pad2C", Keyframe.bin.pad2C);
	}
	tinyxml2::XMLElement* xmlValue = xmlNode->InsertNewChildElement("initial");
	ReadKeyframeCreateVector4Text(xmlValue, Keyframe.bin.initial.m128_f32);
	xmlValue = xmlNode->InsertNewChildElement("velocity");
	ReadKeyframeCreateVector4Text(xmlValue, Keyframe.bin.velocity.m128_f32);

	if (KFCount > 1) {
		xmlValue = xmlNode->InsertNewChildElement("keyframe");
		int kfpos = Keyframe.DataPos;
		switch (Keyframe.bin.type)
		{
		case 1: {
			UINT16 v_kf[3];
			for (int j = 0; j < KFCount; j++) {
				memcpy(v_kf, &buffer[kfpos], 6);
				xmlKF = xmlValue->InsertNewChildElement("v");

#if defined(DEBUGMODE)
				xmlKF->SetAttribute("pos", kfpos);
#endif

				xmlKF->SetAttribute("x", v_kf[0]);
				xmlKF->SetAttribute("y", v_kf[1]);
				xmlKF->SetAttribute("z", v_kf[2]);

				kfpos += 6;
			}
			break;
		}
		case 3: {
			float v_kf[4];
			for (int j = 0; j < KFCount; j++) {
				memcpy(v_kf, &buffer[kfpos], 0x10);
				xmlKF = xmlValue->InsertNewChildElement("v");

#if defined(DEBUGMODE)
				xmlKF->SetAttribute("pos", kfpos);
#endif

				ReadKeyframeCreateVector4Text(xmlKF, v_kf);

				kfpos += 0x10;
			}
			break;
		}
		default:
			break;
		}
	}
}

void CANM6::ReadKeyframeData(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader)
{
	canmKeyframeData_t Keyframe;
	int inputDataCount = header.KeyframeCount;
	v_KeyframeData.reserve(inputDataCount);

#if defined(DEBUGMODE)
	tinyxml2::XMLElement* xmlData = xmlHeader->InsertNewChildElement("KeyframeList");
	tinyxml2::XMLElement* xmlNode;
#endif

	std::wcout << L"Read animation frame list:\n0     in " + ToString(inputDataCount);
	for (int i = 0; i < inputDataCount; i++) {
		int curpos = header.KeyframeOffset + (i * 0x30);

		memcpy(&Keyframe.bin, &buffer[curpos], 0x30);
		Keyframe.CurPos = curpos;
		Keyframe.DataPos = curpos + Keyframe.bin.dataOffset;
		v_KeyframeData.push_back(Keyframe);
		
#if defined(DEBUGMODE)
		xmlNode = xmlData->InsertNewChildElement("value");

		xmlNode->SetAttribute("pos", curpos);
		xmlNode->SetAttribute("datapos", Keyframe.DataPos);
		
		ReadKeyframeDataText(buffer, xmlNode, Keyframe);
#endif

		std::wcout << L"\r" + ToString(i + 1);
	}
	std::wcout << L"\nComplete!\n";
}

void CANM6::ReadBoneListData(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader)
{
	tinyxml2::XMLElement* xmlbone = xmlHeader->InsertNewChildElement("BoneList");
	tinyxml2::XMLElement* xmlptr;
	for (int i = 0; i < header.BoneCount; i++)
	{
		int curpos = header.BoneOffset + (i * 4);

		int boneofs;
		memcpy(&boneofs, &buffer[curpos], 4U);
		xmlptr = xmlbone->InsertNewChildElement("value");

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

void CANM6::ReadKeyframeCreateVector4Text(tinyxml2::XMLElement* xmlNode, const float* value)
{
	xmlNode->SetAttribute("x", value[0]);
	xmlNode->SetAttribute("y", value[1]);
	xmlNode->SetAttribute("z", value[2]);
	xmlNode->SetAttribute("w", value[3]);
}

std::vector<char> CANM6::WriteData(tinyxml2::XMLElement* Data)
{
	tinyxml2::XMLElement* entry, * entry2;

	// read bone name table (if there is)
	std::string bstr;
	entry = Data->FirstChildElement("BoneList");
	if (entry != nullptr) {
		for (entry2 = entry->FirstChildElement("value"); entry2 != 0; entry2 = entry2->NextSiblingElement("value"))
		{
			bstr = entry2->GetText();
			//WBoneList.push_back(UTF8ToWide(bstr));
			WriteWideStringData(bstr);
		}
	}

	// read animation data
	std::wcout << L"Read CANM6 animation list:\n0";
	entry = Data->FirstChildElement("AnmData");
	for (entry2 = entry->FirstChildElement("node"); entry2 != 0; entry2 = entry2->NextSiblingElement("node"))
	{
		// bone data is written at the same time
		v_AnmData.push_back(WriteAnimationData(entry2));
		std::wcout << L"\r" + std::to_wstring(v_AnmData.size());
	}
	std::wcout << L" ===> Complete!\n";


	std::vector< char > bytes(0x20);
	// set header
	header.header = 1296974147; // "CANM"
	header.version = 768;
	header.AnmDataCount = v_AnmData.size();
	header.KeyframeCount = v_KFData.size();
	header.KeyframeOffset = 0x20;
	header.BoneCount = v_BoneList.size();

	// write keyframe data
	std::vector< char > kf_U16, kf_F4;
	std::vector< updateDataOffset_t > v_updateU16x3, v_updateFloat4;
	for (int i = 0; i < v_KFData.size(); i++)
	{
		// set data offset
		if (v_KFData[i].outKF.size())
		{
			updateDataOffset_t out;
			int dataPos = bytes.size();
			out.pos = dataPos + 0x20;

			// check quaternion type
			if (v_KFData[i].type == 3) {
				out.offset = kf_F4.size() - dataPos;
				v_updateFloat4.push_back(out);
				kf_F4.insert(kf_F4.end(), v_KFData[i].outKF.data(), v_KFData[i].outKF.data() + v_KFData[i].outKF.size());
			} else {
				out.offset = kf_U16.size() - dataPos;
				v_updateU16x3.push_back(out);
				kf_U16.insert(kf_U16.end(), v_KFData[i].outKF.data(), v_KFData[i].outKF.data() + v_KFData[i].outKF.size());
			}

			v_KFData[i].outKF.clear();
		}

		bytes.insert(bytes.end(), v_KFData[i].out.data(), v_KFData[i].out.data() + v_KFData[i].out.size());
		v_KFData[i].out.clear();
	}

	// write UINT16 keyframe data
	int i_U16x3Start = bytes.size();
	for (int i = 0; i < v_updateU16x3.size(); i++)
	{
		int str_ofs = i_U16x3Start + v_updateU16x3[i].offset;
		memcpy(&bytes[v_updateU16x3[i].pos], &str_ofs, 4U);
	}
	bytes.insert(bytes.end(), kf_U16.data(), kf_U16.data() + kf_U16.size());
	kf_U16.clear();

	// write FLOAT4 keyframe data
	int i_F4Start = bytes.size();
	// check alignment
	int i_align16 = i_F4Start % 16;
	if (i_align16)
	{
		for (int i = i_align16; i < 16; i++) {
			bytes.push_back(0);
		}
		i_F4Start = bytes.size();
	}
	// write offsets
	for (int i = 0; i < v_updateFloat4.size(); i++)
	{
		int str_ofs = i_F4Start + v_updateFloat4[i].offset;
		memcpy(&bytes[v_updateFloat4[i].pos], &str_ofs, 4U);
	}
	bytes.insert(bytes.end(), kf_F4.data(), kf_F4.data() + kf_F4.size());
	kf_F4.clear();

	// ====================================
	header.AnmDataOffset = bytes.size();
	// write animation data
	std::vector< char > bone_data;
	std::vector< updateDataOffset_t > v_updateData;
	for (int i = 0; i < v_AnmData.size(); i++)
	{
		// set name offset
		if (!v_AnmData[i].name.empty())
		{
			updateDataOffset_t ud_out;
			ud_out.pos = bytes.size();
			// write string data
			ud_out.offset = WriteWideStringToBuffer(v_AnmData[i].name);
			v_updateData.push_back(ud_out);
		}

		// set bone offset
		if (v_AnmData[i].boneData.size())
		{
			int selfPos = (v_AnmData.size() - i) * 0x1C;
			int boneOffset = selfPos + bone_data.size();
			v_AnmData[i].raw.BoneOffset = boneOffset;
			// write bone data
			for (int j = 0; j < v_AnmData[i].boneData.size(); j++)
			{
				bone_data.insert(bone_data.end(), reinterpret_cast<char*>(&v_AnmData[i].boneData[j]), reinterpret_cast<char*>(&v_AnmData[i].boneData[j]) + 8U);
			}
			v_AnmData[i].boneData.clear();
		}

		// write data
		bytes.insert(bytes.end(), reinterpret_cast<char*>(&v_AnmData[i].raw), reinterpret_cast<char*>(&v_AnmData[i].raw) + 0x1Cu);
	}
	// write bone data
	bytes.insert(bytes.end(), bone_data.data(), bone_data.data() + bone_data.size());
	bone_data.clear();

	header.BoneOffset = bytes.size();
	// write bone list
	int boneCount = v_BoneList.size();
	int boneSize = boneCount * 4;
	for (int i = 0; i < boneCount; i++)
	{
		v_BoneList[i] += boneSize;
	}
	bytes.insert(bytes.end(), reinterpret_cast<char*>(v_BoneList.data()), reinterpret_cast<char*>(v_BoneList.data()) + boneCount * 4);

	// update name offsets
	int size_buffer = bytes.size();
	for (int i = 0; i < v_updateData.size(); i++)
	{
		int str_ofs = size_buffer - v_updateData[i].pos + v_updateData[i].offset;
		memcpy(&bytes[v_updateData[i].pos + 4], &str_ofs, 4U);
	}

	// write string data
	bytes.insert(bytes.end(), v_wstirngData.data(), v_wstirngData.data() + v_wstirngData.size());

	// write header
	memcpy(bytes.data(), &header, 0x20U);

	return bytes;
}

int CANM6::WriteWideStringData(const std::string& in)
{
	std::wstring wstr = UTF8ToWide(in);

	// check map
	auto it = map_BoneList.find(wstr);
	if (it != map_BoneList.end()) {
		return it->second;
	}

	// write index
	int index = v_BoneList.size();
	map_BoneList[wstr] = index;

	// write string
	int offset = WriteWideStringToBuffer(wstr);
	offset -= (index * 4);
	v_BoneList.push_back(offset);

	return index;
}

int CANM6::WriteWideStringToBuffer(const std::wstring& in)
{
	int offset = v_wstirngData.size();

	auto strnBytes = reinterpret_cast<const char*>(&in[0]);
	int size = in.size() * 2;

	for (int i = 0; i < size; i += 2)
	{
		v_wstirngData.push_back(strnBytes[i]);
		v_wstirngData.push_back(strnBytes[i + 1]);
	}
	// Zero terminate
	v_wstirngData.push_back(0);
	v_wstirngData.push_back(0);

	return offset;
}

CANM6::outAnimationData_t CANM6::WriteAnimationData(tinyxml2::XMLElement* data)
{
	outAnimationData_t out;

	out.raw.pad0 = data->IntAttribute("loop");
	out.raw.nameOffset = 0xCCCCCCCC;
	std::string name = data->Attribute("name");
	out.name = UTF8ToWide(name);

	out.raw.time = data->FloatAttribute("time");
	out.raw.speed = data->FloatAttribute("speed");
	out.raw.keyframe = data->IntAttribute("keyframe");

	// write bone data
	for (tinyxml2::XMLElement* entry = data->FirstChildElement("value"); entry != 0; entry = entry->NextSiblingElement("value"))
	{
		inBoneData_t boneData;
		std::string bstr = entry->Attribute("bone");
		boneData.bone = WriteWideStringData(bstr);

		boneData.position = WriteAnimationKeyFrame(entry->FirstChildElement("position"));
		boneData.rotation = WriteAnimationKeyFrame(entry->FirstChildElement("rotation"));
		boneData.scaling = WriteAnimationKeyFrame(entry->FirstChildElement("scaling"));

		out.boneData.push_back(boneData);
	}
	out.raw.BoneCount = out.boneData.size();
	out.raw.BoneOffset = 0x1C; // after header
	
	return out;
}

int CANM6::WriteAnimationKeyFrame(tinyxml2::XMLElement* data)
{
	std::string type = data->Attribute("type");
	if (type == "null") {
		return -1;
	}

	outKeyframeData_t out;
	inKeyframeData_t raw;
	raw.type = data->IntAttribute("type");
	out.type = raw.type;
	int keyframe = data->IntAttribute("frame");

	raw.initial = WriteKeyframeCreateVector4(data->FirstChildElement("initial"));
	raw.velocity = WriteKeyframeCreateVector4(data->FirstChildElement("velocity"));

	auto xmlKF = data->FirstChildElement("keyframe");
	if (xmlKF) {
		out.outKF = WriteAnimationKeyFrameData(xmlKF, &keyframe, raw.type);
	} 

	out.keyframe = keyframe;
	raw.keyframe = keyframe;
	raw.dataOffset = -4;
	raw.pad2C = 0;
	out.out.resize(0x30);
	memcpy(out.out.data(), &raw, 0x30U);

	int index = -1;
	for (int i = 0; i < v_KFData.size(); i++)
	{
		if (v_KFData[i].type != out.type) continue;
		if (v_KFData[i].keyframe != out.keyframe) continue;
		if (v_KFData[i].out != out.out) continue;

		if (v_KFData[i].outKF == out.outKF) {
			index = i;
			break;
		}
	}

	if (index != -1) {
		return index;
	}

	index = v_KFData.size();
	out.selfOffset = index * 0x30;
	v_KFData.push_back(out);

	return index;
}

__m128 CANM6::WriteKeyframeCreateVector4(tinyxml2::XMLElement* data)
{
	__m128 out; 
	out.m128_f32[0] = data->FloatAttribute("x");
	out.m128_f32[1] = data->FloatAttribute("y");
	out.m128_f32[2] = data->FloatAttribute("z");
	out.m128_f32[3] = data->FloatAttribute("w");
	return out;
}

std::vector<char> CANM6::WriteAnimationKeyFrameData(tinyxml2::XMLElement* Data, int* keyframe, int type)
{
	std::vector<char> out;
	int kfcount = 0;
	// check is quaternion
	if (type == 3) {
		for (tinyxml2::XMLElement* entry = Data->FirstChildElement("v"); entry != 0; entry = entry->NextSiblingElement("v")) {
			__m128 vf = WriteKeyframeCreateVector4(entry);
			out.insert(out.end(), reinterpret_cast<char*>(&vf), reinterpret_cast<char*>(&vf) + 0x10);
			kfcount++;
		}
	} else {
		UINT16 v_kf[3];
		for (tinyxml2::XMLElement* entry = Data->FirstChildElement("v"); entry != 0; entry = entry->NextSiblingElement("v")) {
			v_kf[0] = entry->IntAttribute("x");
			v_kf[1] = entry->IntAttribute("y");
			v_kf[2] = entry->IntAttribute("z");
			out.insert(out.end(), reinterpret_cast<char*>(v_kf), reinterpret_cast<char*>(v_kf) + 6);
			kfcount++;
		}
	}

	*keyframe = kfcount;
	return out;
}
