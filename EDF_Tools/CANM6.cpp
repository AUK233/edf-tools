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
