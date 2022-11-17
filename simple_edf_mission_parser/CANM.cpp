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

void CANM::ReadData(std::vector<char> buffer, tinyxml2::XMLElement* header)
{
	int position = 0;
	unsigned char seg[4];
	// read header, length is 0x20
	memcpy(&seg, &buffer[0], 4U);
	//if (seg[0] == 0x43 && seg[1] == 0x41 && seg[2] == 0x4E && seg[3] == 0x4D)	
	
	// read AnmData
	memcpy(&i_AnmDataCount, &buffer[0x8], 4U);
	memcpy(&i_AnmDataOffset, &buffer[0xC], 4U);
	// read AnmPoint
	memcpy(&i_AnmPointCount, &buffer[0x10], 4U);
	memcpy(&i_AnmPointOffset, &buffer[0x14], 4U);
	// read bone
	memcpy(&i_BoneCount, &buffer[0x18], 4U);
	memcpy(&i_BoneOffset, &buffer[0x1C], 4U);

	// animation data
	ReadAnimationData(header, buffer);
	// animation key data
	ReadAnimationKeyData(header, buffer);
	// bone data
	ReadBoneListData(header, buffer);
}

void CANM::ReadAnimationData(tinyxml2::XMLElement* header, std::vector<char> buffer)
{
	tinyxml2::XMLElement* xmldata = header->InsertNewChildElement("AnmData");
	for (int i = 0; i < i_AnmDataCount; i++)
	{
		int curpos = i_AnmDataOffset + (i * 0x1C);

		int value[7];
		memcpy(&value, &buffer[curpos], 28U);
		tinyxml2::XMLElement* xmlptr = xmldata->InsertNewChildElement("node");

#if defined(DEBUGMODE)
		xmlptr->SetAttribute("index", i);
		xmlptr->SetAttribute("pos", curpos);
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
		xmlptr->SetAttribute("float1", IntHexAsFloat(value[2]));
		// i3 is float
		xmlptr->SetAttribute("float2", IntHexAsFloat(value[3]));
		// i4 is int.
		xmlptr->SetAttribute("int2", value[4]);

		// i5 is amount, i6 is offset
		for (int j = 0; j < value[5]; j++)
		{
			int datapos = curpos + value[6] + (j * 8);

			short number[4];
			memcpy(&number, &buffer[datapos], 8U);

			tinyxml2::XMLElement* xmlNode = xmlptr->InsertNewChildElement("value");
			xmlNode->SetAttribute("short1", number[0]);
			xmlNode->SetAttribute("short2", number[1]);
			xmlNode->SetAttribute("short3", number[2]);
			xmlNode->SetAttribute("short4", number[3]);
		}
	}
	// end
}

void CANM::ReadAnimationKeyData(tinyxml2::XMLElement* header, std::vector<char> buffer)
{
	tinyxml2::XMLElement* xmldata = header->InsertNewChildElement("AnmKey");
	for (int i = 0; i < i_AnmPointCount; i++)
	{
		int curpos = i_AnmPointOffset + (i * 0x20);

		short value[2];
		memcpy(&value, &buffer[curpos], 4U);
		tinyxml2::XMLElement* xmlptr = xmldata->InsertNewChildElement("node");

#if defined(DEBUGMODE)
		xmlptr->SetAttribute("index", i);
		xmlptr->SetAttribute("pos", curpos);
#endif

		xmlptr->SetAttribute("int1", value[0]);
		xmlptr->SetAttribute("int2", value[1]);

		// get half
		for (int j = 0; j < 3; j++)
		{
			int datapos = curpos + 4 + (j * 8);

			half_float::half vf[4];
			memcpy(&vf, &buffer[datapos], 8U);

			tinyxml2::XMLElement* xmlNode = xmlptr->InsertNewChildElement("value");

			xmlNode->SetAttribute("x", vf[0]);
			xmlNode->SetAttribute("y", vf[1]);
			xmlNode->SetAttribute("z", vf[2]);
			xmlNode->SetAttribute("w", vf[3]);
		}

		// unknown
		int unk[2];
		memcpy(&unk, &buffer[curpos + 24], 8U);

		xmlptr->SetAttribute("unk1", unk[0]);
		xmlptr->SetAttribute("unk2", unk[1]);
	}
	// end
}

void CANM::ReadBoneListData(tinyxml2::XMLElement* header, std::vector<char> buffer)
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
	}
}
