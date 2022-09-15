#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>

#include <iostream>
#include <locale>
#include <locale.h>

#include "Middleware.h"
#include "util.h"
#include "MAB.h"
#include "include/tinyxml2.h"

//Read data from MAB
void MAB::Read(std::wstring path)
{
	std::ifstream file(path + L".mab", std::ios::binary | std::ios::ate | std::ios::in);

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	if (file.read(buffer.data(), size))
	{
		// create xml
		tinyxml2::XMLDocument xml = new tinyxml2::XMLDocument();
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

void MAB::ReadData(std::vector<char> buffer, tinyxml2::XMLElement* header, tinyxml2::XMLElement* xmlHeader)
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

void MAB::ReadBoneData(std::vector<char>& buffer, int curpos, tinyxml2::XMLElement* xmlBone, tinyxml2::XMLElement* xmlHeader)
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
		// read extra
		std::string namestr = "MAB_" + std::to_string(buffer.size()) + "_" + std::to_string(vi[1]);
		xmlBPtr->SetAttribute("extra", namestr.c_str());
		ReadExtraSGO(namestr, buffer, vi[1], xmlHeader);
	}
	// end
}

void MAB::ReadBoneTypeData(int type, std::vector<char>& buffer, int ptrpos, tinyxml2::XMLElement* xmlBPtr)
{
	if (type == 0)
	{
		tinyxml2::XMLElement* xmlNode;

		// get offset of float group
		int fofs;
		memcpy(&fofs, &buffer[ptrpos + 0xC], 4U);
		// set float group
		xmlNode = xmlBPtr->InsertNewChildElement("floatgroup");
		xmlNode->SetAttribute("name", "a");
		float vf[4];
		memcpy(&vf, &buffer[fofs], 16U);
		Read4FloatData(xmlNode, vf);

		// get float
		xmlNode = xmlBPtr->InsertNewChildElement("float");
		float fvalue;
		memcpy(&fvalue, &buffer[ptrpos + 0x10], 4U);
		xmlNode->SetText(fvalue);

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
				xmlNode->SetAttribute("name", "a");
				break;
			case 1:
				xmlNode->SetAttribute("name", "b");
				break;
			case 2:
				xmlNode->SetAttribute("name", "c");
				break;
			default:
				break;
			}
			// get float
			Read4FloatData(xmlNode, vf);
		}
	}
	else
	{
		std::wcout << L"Unknown type at position: " + ToString(ptrpos + 8) + L"\n";
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

void MAB::ReadExtraSGO(std::string& namestr, std::vector<char>& buffer, int pos, tinyxml2::XMLElement*& xmlHeader)
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

		std::vector<char> newbuf;
		// no data size, so copy remain
		int filesize = buffer.size() - pos;
		for (int i = 0; i < filesize; i++)
			newbuf.push_back(buffer[pos + i]);

		CheckDataType(newbuf, xmlHeader, namestr);
	}
}

void MAB::ReadAnimeData(std::vector<char>& buffer, int curpos, tinyxml2::XMLElement* xmlAnm, tinyxml2::XMLElement* xmlHeader)
{
	// get value
	int value[4];
	memcpy(&value, &buffer[curpos], 16U);
	// output data
	tinyxml2::XMLElement* xmlANode = xmlAnm->InsertNewChildElement("value");
	xmlANode->SetAttribute("debugpos", curpos);

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

void MAB::ReadAnimeDataA(std::vector<char>& buffer, int pos, tinyxml2::XMLElement* xmlNode, tinyxml2::XMLElement* xmlHeader)
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
