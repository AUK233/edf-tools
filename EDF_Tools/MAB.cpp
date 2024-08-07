#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <algorithm>

#include "Middleware.h"
#include "util.h"
#include "MAB.h"
#include "include/tinyxml2.h"

//Read data from MAB
void MAB::Read(const std::wstring& path)
{
	std::ifstream file(path + L".mab", std::ios::binary | std::ios::ate | std::ios::in);

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	if (file.read(buffer.data(), size))
	{
		// create xml
		tinyxml2::XMLDocument xml;
		xml.InsertFirstChild(xml.NewDeclaration());
		tinyxml2::XMLElement* xmlHeader = xml.NewElement("EDFDATA");
		xml.InsertEndChild(xmlHeader);

		tinyxml2::XMLElement* xmlMain = xmlHeader->InsertNewChildElement("Main");
		xmlMain->SetAttribute("header", "MAB");

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
	buffer.clear();
	file.close();
}

void MAB::ReadData(const std::vector<char>& buffer, tinyxml2::XMLElement* header, tinyxml2::XMLElement* xmlHeader)
{
	int position = 0;
	unsigned char seg[4];
	// read header, length is 0x24
	memcpy(&seg, &buffer[0], 4U);
	//if (seg[0] == 0x4D && seg[1] == 0x41 && seg[2] == 0x42 && seg[3] == 0x00)

	// get count
	memcpy(&BoneCount, &buffer[0xC], 2U);
	memcpy(&AnimationCount, &buffer[0xE], 2U);
	memcpy(&FloatGroupCount, &buffer[0x10], 2U);
	memcpy(&StringSize, &buffer[0x12], 2U);
	// get offset
	memcpy(&BoneOffset, &buffer[0x14], 4U);
	memcpy(&AnimationOffset, &buffer[0x18], 4U);
	memcpy(&FloatGroupOffset, &buffer[0x1C], 4U);
	memcpy(&StringOffset, &buffer[0x20], 4U);

	// read bone or point
	tinyxml2::XMLElement* xmlBone = header->InsertNewChildElement("Bone");

	position = BoneOffset;
	for (int i = 0; i < BoneCount; i++)
	{
		int curpos = position + (i * 8);
		ReadBoneData(buffer, curpos, xmlBone, xmlHeader);
	}

	// read animation
	tinyxml2::XMLElement* xmlAnm = header->InsertNewChildElement("Anime");

	position = AnimationOffset;
	for (int i = 0; i < AnimationCount; i++)
	{
		int curpos = position + (i * 0x10);
		ReadAnimeData(buffer, curpos, xmlAnm, xmlHeader);
	}
}

void MAB::ReadBoneData(const std::vector<char>& buffer, int curpos, tinyxml2::XMLElement* xmlBone, tinyxml2::XMLElement* xmlHeader)
{
	// get int16
	short value[2];
	memcpy(&value, &buffer[curpos], 4U);
	// get offset
	int offset;
	memcpy(&offset, &buffer[curpos + 4], 4U);
	// output data
	tinyxml2::XMLElement* xmlBNode = xmlBone->InsertNewChildElement("value");
	xmlBNode->SetAttribute("ID", value[0]);
	// get parameters
	for (int i = 0; i < value[1]; i++)
	{
		int ptrpos = offset + (i * 0x20);
		tinyxml2::XMLElement* xmlBPtr = xmlBNode->InsertNewChildElement("ptr");
		//xmlBPtr->SetAttribute("debugpos", ptrpos);

		// get string offset
		int strofs[2];
		memcpy(&strofs, &buffer[ptrpos], 8U);
		// get string
		std::wstring wstr;
		std::string utf8str;
		// str1
		if (strofs[0] > 0)
			wstr = ReadUnicode(buffer, strofs[0]);
		else
			wstr = L"";
		utf8str = WideToUTF8(wstr);
		xmlBPtr->SetAttribute("ExportBone", utf8str.c_str());
		// str2
		if (strofs[1] > 0)
			wstr = ReadUnicode(buffer, strofs[1]);
		else
			wstr = L"";
		utf8str = WideToUTF8(wstr);
		xmlBPtr->SetAttribute("Parent", utf8str.c_str());

		// get type
		int type;
		memcpy(&type, &buffer[ptrpos + 8], 4U);
		xmlBPtr->SetAttribute("Type", type);

		ReadBoneTypeData(type, buffer, ptrpos, xmlBPtr);

		// unknown
		int vi[2];
		memcpy(&vi, &buffer[ptrpos + 0x18], 8U);
		xmlBPtr->SetAttribute("unknown", vi[0]);
		if (vi[0] != 0)
			std::wcout << L"Unknown value at position: " + ToString(ptrpos + 8) + L" - value: " + ToString(vi[0]) + L"\n";

		// read extra
		std::string namestr = "MAB_" + std::to_string(buffer.size()) + "_" + std::to_string(vi[1]);
		xmlBPtr->SetAttribute("extra", namestr.c_str());
		ReadExtraSGO(namestr, buffer, vi[1], xmlHeader);
	}
	// end
}

void MAB::ReadBoneTypeData(int type, const std::vector<char>& buffer, int ptrpos, tinyxml2::XMLElement* xmlBPtr)
{
	if (type == 0)
	{
		tinyxml2::XMLElement* xmlNode;

		// get offset of float group
		int fofs;
		memcpy(&fofs, &buffer[ptrpos + 0xC], 4U);
		// set float group
		xmlNode = xmlBPtr->InsertNewChildElement("floatgroup");
		xmlNode->SetAttribute("name", "location");
		float vf[4];
		memcpy(&vf, &buffer[fofs], 16U);
		Read4FloatData(xmlNode, vf);

		// get float
		xmlNode = xmlBPtr->InsertNewChildElement("float");
		xmlNode->SetAttribute("name", "scale");
		float fvalue;
		memcpy(&fvalue, &buffer[ptrpos + 0x10], 4U);
		xmlNode->SetText(fvalue);

		// get int
		xmlNode = xmlBPtr->InsertNewChildElement("int");
		int ivalue;
		memcpy(&ivalue, &buffer[ptrpos + 0x14], 4U);
		xmlNode->SetText(ivalue);
	}
	else if (type == 1)
	{
		tinyxml2::XMLElement* xmlNode;

		// get offset of float group
		int fofs[2];
		memcpy(&fofs, &buffer[ptrpos + 0xC], 8U);
		// set float group
		for (int j = 0; j < 2; j++)
		{
			xmlNode = xmlBPtr->InsertNewChildElement("floatgroup");
			float vf[4];
			memcpy(&vf, &buffer[fofs[j]], 16U);
			// output type help
			switch (j)
			{
			case 0:
				xmlNode->SetAttribute("name", "location");
				break;
			case 1:
				xmlNode->SetAttribute("name", "scale");
				break;
			default:
				break;
			}
			// get float
			Read4FloatData(xmlNode, vf);
		}

		// get int
		xmlNode = xmlBPtr->InsertNewChildElement("int");
		int ivalue;
		memcpy(&ivalue, &buffer[ptrpos + 0x14], 4U);
		xmlNode->SetText(ivalue);
	}
	else if (type == 2)
	{
		// get offset of float group
		int fofs[3];
		memcpy(&fofs, &buffer[ptrpos + 0xC], 12U);
		// set float group
		for (int j = 0; j < 3; j++)
		{
			tinyxml2::XMLElement* xmlNode = xmlBPtr->InsertNewChildElement("floatgroup");
			float vf[4];
			memcpy(&vf, &buffer[fofs[j]], 16U);
			// output type help
			switch (j)
			{
			case 0:
				xmlNode->SetAttribute("name", "location");
				break;
			case 1:
				xmlNode->SetAttribute("name", "scale");
				break;
			case 2:
				xmlNode->SetAttribute("name", "rotation");
				break;
			default:
				break;
			}
			// get float
			Read4FloatData(xmlNode, vf);
		}
	}
	else if (type == 3 || type == 4)
	{
		tinyxml2::XMLElement* xmlNode;

		// get offset of float group 1
		int fofs1;
		memcpy(&fofs1, &buffer[ptrpos + 0xC], 4U);
		// set float group
		xmlNode = xmlBPtr->InsertNewChildElement("floatgroup");
		xmlNode->SetAttribute("name", "location");
		float vf1[4];
		memcpy(&vf1, &buffer[fofs1], 16U);
		Read4FloatData(xmlNode, vf1);

		// get float
		xmlNode = xmlBPtr->InsertNewChildElement("float");
		float fvalue;
		memcpy(&fvalue, &buffer[ptrpos + 0x10], 4U);
		xmlNode->SetText(fvalue);

		// get offset of float group 2
		int fofs2;
		memcpy(&fofs2, &buffer[ptrpos + 0x14], 4U);
		// set float group
		xmlNode = xmlBPtr->InsertNewChildElement("floatgroup");
		xmlNode->SetAttribute("name", "c");
		float vf2[4];
		memcpy(&vf2, &buffer[fofs2], 16U);
		Read4FloatData(xmlNode, vf2);
	}
	else
	{
		std::wcout << L"Unknown type at position: " + ToString(ptrpos + 8) + L" - ";
		std::wcout << L"Type: " + ToString(type) + L"\n";
	}
}

void MAB::Read4FloatData(tinyxml2::XMLElement* xmlNode, float* vf)
{
	for (int i = 0; i < 4; i++)
	{
		tinyxml2::XMLElement* xmlvalue = xmlNode->InsertNewChildElement("value");
		xmlvalue->SetText(vf[i]);
	}
	/*
	xmlvalue->SetAttribute("x", vf[0]);
	xmlvalue->SetAttribute("y", vf[1]);
	xmlvalue->SetAttribute("z", vf[2]);
	xmlvalue->SetAttribute("w", vf[3]);
	*/
}

void MAB::ReadExtraSGO(std::string& namestr, const std::vector<char>& buffer, int pos, tinyxml2::XMLElement*& xmlHeader)
{
	// check for duplicate data
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

		// no data size, so copy remain
		std::vector<char> newbuf(buffer.begin() + pos, buffer.end());

		CheckDataType(newbuf, xmlHeader, namestr);
	}
}

void MAB::ReadAnimeData(const std::vector<char>& buffer, int curpos, tinyxml2::XMLElement* xmlAnm, tinyxml2::XMLElement* xmlHeader)
{
	// get value
	int value[4];
	memcpy(&value, &buffer[curpos], 16U);
	// output data
	tinyxml2::XMLElement* xmlANode = xmlAnm->InsertNewChildElement("value");
	//xmlANode->SetAttribute("debugpos", curpos);

	// value[2] is string
	tinyxml2::XMLElement* xmlstr = xmlANode->InsertNewChildElement("name");
	if (value[2] > 0)
	{
		std::wstring wstr = ReadUnicode(buffer, value[2]);
		std::string utf8str = WideToUTF8(wstr);
		xmlstr->SetText(utf8str.c_str());
	}

	// value[3] is count
	tinyxml2::XMLElement* xmlANode1 = xmlANode->InsertNewChildElement("ptrA");
	if (value[3] > 0)
	{
		for (int i = 0; i < value[3]; i++)
		{
			// value[0] is offset
			int newpos = value[0] + (i * 0x10);
			ReadAnimeDataA(buffer, newpos, xmlANode1, xmlHeader);
		}
	}

	// value[1] is offset
	tinyxml2::XMLElement* xmlANode2 = xmlANode->InsertNewChildElement("ptrB");
	if (value[1] > 0)
	{
		unsigned int check;
		memcpy(&check, &buffer[value[1]], 4U);
		if (check != 0xBABABABA)
			std::wcout << L"Check the pointed data: " + ToString(value[1]) + L"\n";
	}
}

void MAB::ReadAnimeDataA(const std::vector<char>& buffer, int pos, tinyxml2::XMLElement* xmlNode, tinyxml2::XMLElement* xmlHeader)
{
	tinyxml2::XMLElement* xmlptr = xmlNode->InsertNewChildElement("value");
	//xmlptr->SetAttribute("debugpos", pos);

	// get name
	int strofs;
	memcpy(&strofs, &buffer[pos], 4U);

	std::wstring wstr = ReadUnicode(buffer, strofs);
	std::string utf8str = WideToUTF8(wstr);
	xmlptr->SetAttribute("name", utf8str.c_str());

	// set float
	float vf;
	memcpy(&vf, &buffer[pos + 4], 4U);
	xmlptr->SetAttribute("float", vf);

	// set unknown
	int unk;
	memcpy(&unk, &buffer[pos + 8], 4U);
	xmlptr->SetAttribute("unk", unk);

	// get sgo
	int sgoofs;
	memcpy(&sgoofs, &buffer[pos + 12], 4U);

	std::string namestr = "MAB_" + std::to_string(buffer.size()) + "_" + std::to_string(sgoofs);
	xmlptr->SetAttribute("extra", namestr.c_str());
	ReadExtraSGO(namestr, buffer, sgoofs, xmlHeader);
}

void MAB::Write(const std::wstring& path, tinyxml2::XMLNode* header)
{
	std::wcout << "Will output MAB file.\n";

	tinyxml2::XMLElement* mainData = header->FirstChildElement("Main");

	std::vector< char > bytes;
	bytes = WriteData(mainData, header);

	//Final write.
	/**/
	std::ofstream newFile(path + L".mab", std::ios::binary | std::ios::out | std::ios::ate);

	newFile.write(bytes.data(), bytes.size());

	newFile.close();
	
	std::wcout << L"Conversion completed: " + path + L".mab\n";
}

std::vector<char> MAB::WriteData(tinyxml2::XMLElement* mainData, tinyxml2::XMLNode* header)
{
	std::vector< char > bytes;

	tinyxml2::XMLElement *entry, *entry2, *entry3, *entry4;

	// prefetch data size
	std::string namestr;
	// get bone count
	int BonePtrNum = 0;
	entry = mainData->FirstChildElement("Bone");
	for (entry2 = entry->FirstChildElement("value"); entry2 != 0; entry2 = entry2->NextSiblingElement("value"))
	{
		for (entry3 = entry2->FirstChildElement("ptr"); entry3 != 0; entry3 = entry3->NextSiblingElement("ptr"))
		{
			namestr = entry3->Attribute("ExportBone");
			GetMABString(namestr);

			namestr = entry3->Attribute("Parent");
			GetMABString(namestr);

			GetMABExtraDataName(entry3);

			BonePtrNum++;
		}

		BoneCount++;
	}
	// get anime count
	int animePtrNum = 0;
	entry = mainData->FirstChildElement("Anime");
	for (entry2 = entry->FirstChildElement("value"); entry2 != 0; entry2 = entry2->NextSiblingElement("value"))
	{
		entry3 = entry2->FirstChildElement("name");
		namestr = entry3->GetText();
		GetMABString(namestr);

		entry3 = entry2->FirstChildElement("ptrA");
		for (entry4 = entry3->FirstChildElement("value"); entry4 != 0; entry4 = entry4->NextSiblingElement("value"))
		{
			namestr = entry4->Attribute("name");
			GetMABString(namestr);

			GetMABExtraDataName(entry4);

			animePtrNum++;
		}

		AnimationCount++;
	}

	// generate size
	int i_headerSize = 0x24;
	// bone
	BoneOffset = i_headerSize;
	int i_boneSize = BoneCount * 0x8;
	int i_bonePtrOfs = BoneOffset + i_boneSize;
	int i_bonePtrSize = BonePtrNum * 0x20;
	// anime
	AnimationOffset = i_bonePtrOfs + i_bonePtrSize;
	int i_animeSize = AnimationCount * 0x10;
	int i_animePtrOfs = AnimationOffset + i_animeSize;
	int i_animePtrSize = animePtrNum * 0x10;
	// null pointer: 0xBABABABA
	int i_nullSize = i_animePtrOfs + i_animePtrSize + 4;
	int a_nullSize = i_nullSize;
	if (i_nullSize % 16 != 0)
		a_nullSize = (i_nullSize / 16 + 1) * 16;

	// prefetch float data
	FloatGroupOffset = a_nullSize;
	entry = mainData->FirstChildElement("Bone");
	for (entry2 = entry->FirstChildElement("value"); entry2 != 0; entry2 = entry2->NextSiblingElement("value"))
	{
		for (entry3 = entry2->FirstChildElement("ptr"); entry3 != 0; entry3 = entry3->NextSiblingElement("ptr"))
		{
			GetMABFloatGroup(entry3);
		}
	}
	int i_floatSize = (FloatGroup.size() * 0x10);

	// prefetch extra data
	std::string dataName;
	int i_extraOfs = a_nullSize + i_floatSize;
	int i_extraPos = i_extraOfs;
	for (entry = header->FirstChildElement("Subdata"); entry != 0; entry = entry->NextSiblingElement("Subdata"))
	{
		dataName = entry->Attribute("name");
		for (size_t i = 0; i < SubDataGroup.size(); i++)
		{
			if (SubDataGroup[i] == dataName)
			{
				ExtraData.push_back(GetExtraData(entry, dataName, header, i_extraPos));
				i_extraPos += ExtraData.back().bytes.size();
				break;
			}
		}
	}

	// out string
	StringOffset = i_extraPos;
	std::sort(NodeString.begin(), NodeString.end());
	int strpos = 0;
	for (size_t i = 0; i < NodeString.size(); i++)
	{
		MABString NN;
		NN.pos = StringOffset + strpos;
		NN.name = UTF8ToWide(NodeString[i]);
		NodeWString.push_back(NN);
		// Must be converted to UTF16 first, because UTF8 is not fixed length.
		strpos += (NN.name.size() * 2);
		strpos += 2;
	}
	StringSize = strpos;

	// get bone data
	entry = mainData->FirstChildElement("Bone");
	for (entry2 = entry->FirstChildElement("value"); entry2 != 0; entry2 = entry2->NextSiblingElement("value"))
	{
		boneData.push_back(GetMABBoneData(entry2, i_bonePtrOfs));
	}

	//get anime data
	entry = mainData->FirstChildElement("Anime");
	for (entry2 = entry->FirstChildElement("value"); entry2 != 0; entry2 = entry2->NextSiblingElement("value"))
	{
		animeData.push_back(GetMABAnimeData(entry2, i_animePtrOfs, i_nullSize-4));
	}

	// push bytes!
	bytes.resize(0x24, 0);
	bytes[0] = 0x4D;
	bytes[1] = 0x41;
	bytes[2] = 0x42;
	bytes[4] = 0x0F;
	bytes[8] = 0x03;
	// count
	memcpy(&bytes[0xC], &BoneCount, 2U);
	memcpy(&bytes[0xE], &AnimationCount, 2U);
	memcpy(&bytes[0x10], &FloatGroupCount, 2U);
	memcpy(&bytes[0x12], &StringSize, 2U);
	//offset
	bytes[0x14] = 0x24;
	memcpy(&bytes[0x18], &AnimationOffset, 4U);
	memcpy(&bytes[0x1C], &FloatGroupOffset, 4U);
	memcpy(&bytes[0x20], &StringOffset, 4U);

	// write bone
	for (size_t i = 0; i < boneData.size(); i++)
	{
		bytes.insert(bytes.end(), boneData[i].bytes.begin(), boneData[i].bytes.end());
	}

	for (size_t i = 0; i < bonePtrData.size(); i++)
	{
		bytes.insert(bytes.end(), bonePtrData[i].bytes.begin(), bonePtrData[i].bytes.end());
	}

	// write anime
	for (size_t i = 0; i < animeData.size(); i++)
	{
		bytes.insert(bytes.end(), animeData[i].bytes.begin(), animeData[i].bytes.end());
	}

	for (size_t i = 0; i < animePtrData.size(); i++)
	{
		bytes.insert(bytes.end(), animePtrData[i].bytes.begin(), animePtrData[i].bytes.end());
	}

	// write null pointer
	for (int i = 0; i < 4; i++)
		bytes.push_back(0xBA);
	// check alignment
	if (a_nullSize > i_nullSize)
	{
		for (int i = (a_nullSize - i_nullSize); i > 0; --i)
			bytes.push_back(0xBA);
	}

	// write float group
	for (size_t i = 0; i < FloatGroup.size(); i++)
	{
		bytes.insert(bytes.end(), FloatGroup[i].bytes.begin(), FloatGroup[i].bytes.end());
	}

	// write extra file
	for (size_t i = 0; i < ExtraData.size(); i++)
	{
		bytes.insert(bytes.end(), ExtraData[i].bytes.begin(), ExtraData[i].bytes.end());
	}
	// write wide string
	for (size_t i = 0; i < NodeWString.size(); i++)
		PushWStringToVector(NodeWString[i].name, &bytes);


	// debug only
	/*
	std::wcout << L"BoneCount: " + ToString(BoneCount) + L"\n";
	std::wcout << L"BonePtrNum: " + ToString(BonePtrNum) + L"\n";
	std::wcout << L"AnimationCount: " + ToString(AnimationCount) + L"\n";
	std::wcout << L"animePtrNum: " + ToString(animePtrNum) + L"\n";

	std::wcout << L"FloatGroupOffset: " + ToString(FloatGroupOffset) + L"\n";
	std::wcout << L"FloatGroupCount: " + ToString(FloatGroupCount) + L"\n";
	std::wcout << L"FloatGroupSize: " + ToString(i_floatSize) + L"\n";

	std::wcout << L"Extra Data End: " + ToString(i_extraPos) + L"\n";
	*/
	return bytes;
}

void MAB::GetMABString(std::string namestr)
{
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

void MAB::GetMABExtraDataName(tinyxml2::XMLElement* data)
{
	std::string namestr = data->Attribute("extra");
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

void MAB::GetMABFloatGroup(tinyxml2::XMLElement* entry3)
{
	for (tinyxml2::XMLElement* fg = entry3->FirstChildElement("floatgroup"); fg != 0; fg = fg->NextSiblingElement("floatgroup"))
	{
		MABFloatGroup out;
		// get float 4
		int vfCount = 0;
		for (tinyxml2::XMLElement* vf = fg->FirstChildElement("value"); vfCount < 4; vf = vf->NextSiblingElement("value"))
		{
			out.f[vfCount] = vf->FloatText();
			vfCount++;
		}
		// get pos
		out.pos = FloatGroupOffset + (FloatGroup.size() * 0x10);
		// output bytes
		out.bytes.resize(0x10);
		memcpy(&out.bytes[0], &out.f, 16U);
		// check for duplicates
		bool isExist = false;
		for (size_t i = 0; i < FloatGroup.size(); i++)
		{
			if (FloatGroup[i].bytes == out.bytes)
			{
				isExist = true;
				break;
			}
		}

		if (!isExist)
		{
			FloatGroup.push_back(out);
			FloatGroupCount++;
		}
	}
	// end
}

MABExtraData MAB::GetExtraData(tinyxml2::XMLElement* entry, std::string dataName, tinyxml2::XMLNode* header, int pos)
{
	MABExtraData out;
	out.name = dataName;
	out.bytes = CheckDataType(entry, header);
	out.pos = pos;
	return out;
}

int MAB::GetMABStringOffset(const std::string& namestr)
{
	int pos = 0;

	std::wstring wstr = UTF8ToWide(namestr);
	for (size_t i = 0; i < NodeWString.size(); i++)
	{
		if (NodeWString[i].name == wstr)
		{
			pos = NodeWString[i].pos;
			break;
		}
	}

	return pos;
}

int MAB::GetMABExtraOffset(const std::string& namestr)
{
	int pos = 0;

	for (size_t i = 0; i < ExtraData.size(); i++)
	{
		if (ExtraData[i].name == namestr)
		{
			pos = ExtraData[i].pos;
			break;
		}
	}

	return pos;
}

MABData MAB::GetMABBoneData(tinyxml2::XMLElement* entry2, int ptrpos)
{
	MABData out;

	short value[2];
	value[0] = entry2->IntAttribute("ID");
	value[1] = 0;
	int offset = ptrpos + (bonePtrData.size() * 0x20);
	// get ptr
	for (tinyxml2::XMLElement* entry3 = entry2->FirstChildElement("ptr"); entry3 != 0; entry3 = entry3->NextSiblingElement("ptr"))
	{
		bonePtrData.push_back(GetMABBonePtrData(entry3));
		value[1] += 1;
	}
	// push bytes
	out.bytes.resize(0x8);
	memcpy(&out.bytes[0], &value, 4U);
	memcpy(&out.bytes[4], &offset, 4U);

	return out;
}

MABData MAB::GetMABBonePtrData(tinyxml2::XMLElement* entry3)
{
	MABData out;
	out.bytes.resize(0x20);
	// get string
	int pos[2];
	std::string str;
	// 1
	str = entry3->Attribute("ExportBone");
	pos[0] = GetMABStringOffset(str);
	// 2
	str = entry3->Attribute("Parent");
	pos[1] = GetMABStringOffset(str);
	memcpy(&out.bytes[0], &pos, 8U);

	// get type and value
	int type;
	type = entry3->IntAttribute("Type");
	memcpy(&out.bytes[0x8], &type, 4U);
	// type is not checked now
	//if (type == 0)
	int ptr[3];
	tinyxml2::XMLElement* entry4 = entry3->FirstChildElement();
	ptr[0] = GetMABBonePtrValue(entry4);

	entry4 = entry4->NextSiblingElement();
	ptr[1] = GetMABBonePtrValue(entry4);

	entry4 = entry4->NextSiblingElement();
	ptr[2] = GetMABBonePtrValue(entry4);

	memcpy(&out.bytes[0xC], &ptr, 12U);

	// get unknown and extra
	int value[2];
	value[0] = entry3->IntAttribute("unknown");
	// get extra
	str = entry3->Attribute("extra");
	value[1] = GetMABExtraOffset(str);
	memcpy(&out.bytes[0x18], &value, 8U);

	return out;
}

int MAB::GetMABBonePtrValue(tinyxml2::XMLElement* entry)
{
	int out = 0;

	std::string nodeType = entry->Name();
	if (nodeType == "floatgroup")
	{
		// get float 4
		float vf[4];
		int count = 0;
		for (tinyxml2::XMLElement* node = entry->FirstChildElement("value"); count < 4; node = node->NextSiblingElement("value"))
		{
			vf[count] = node->FloatText();
			count++;
		}
		// get pos
		std::vector<char> temp(16U);
		memcpy(&temp[0], &vf, 16U);
		// check for duplicates
		for (size_t i = 0; i < FloatGroup.size(); i++)
		{
			if (FloatGroup[i].bytes == temp)
			{
				out = FloatGroup[i].pos;
				break;
			}
		}
	}
	else if (nodeType == "int")
	{
		out = entry->IntText();
	}
	else if (nodeType == "float")
	{
		float vf = entry->FloatText();
		memcpy(&out, &vf, 4U);
	}

	return out;
}

MABData MAB::GetMABAnimeData(tinyxml2::XMLElement* entry2, int ptrpos, int nullpos)
{
	MABData out;

	tinyxml2::XMLElement* entry3;
	int offset[4];

	// get string
	entry3 = entry2->FirstChildElement("name");
	std::string str = entry3->GetText();
	offset[2] = GetMABStringOffset(str);

	// get ptr 1
	// initialize the count to 0
	offset[3] = 0;
	entry3 = entry2->FirstChildElement("ptrA");
	if (entry3->FirstChildElement("value"))
	{
		offset[0] = ptrpos + (animePtrData.size() * 0x10);;
		for (tinyxml2::XMLElement* entry4 = entry3->FirstChildElement("value"); entry4 != 0; entry4 = entry4->NextSiblingElement("value"))
		{
			animePtrData.push_back(GetMABAnimePtrData(entry4, nullpos));
			offset[3] += 1;
		}
	}
	else
	{
		offset[0] = nullpos;
	}

	// get ptr 2, now points to null
	offset[1] = nullpos;

	// push bytes
	out.bytes.resize(0x10);
	memcpy(&out.bytes[0], &offset, 16U);

	return out;
}

MABData MAB::GetMABAnimePtrData(tinyxml2::XMLElement* entry3, int nullpos)
{
	MABData out;
	std::string str;

	out.bytes.resize(0x10);

	// get string
	str = entry3->Attribute("name");
	int pos = GetMABStringOffset(str);
	memcpy(&out.bytes[0], &pos, 4U);

	// get float
	float vf = entry3->FloatAttribute("float");
	memcpy(&out.bytes[4], &vf, 4U);

	// get unknown and extra
	int value[2];
	value[0] = entry3->IntAttribute("unk");
	// get extra
	str = entry3->Attribute("extra");
	value[1] = GetMABExtraOffset(str);
	memcpy(&out.bytes[8], &value, 8U);

	return out;
}
