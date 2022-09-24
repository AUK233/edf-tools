#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <codecvt>
#include <sstream>

#include <iostream>
#include <locale>
#include "util.h"
#include "CAS.h"
#include "include/tinyxml2.h"

void CAS::Read(std::wstring path)
{
	std::ifstream file(path + L".cas", std::ios::binary | std::ios::ate | std::ios::in);

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	if (file.read(buffer.data(), size))
	{
		// create xml
		tinyxml2::XMLDocument xml = new tinyxml2::XMLDocument();
		xml.InsertFirstChild(xml.NewDeclaration());
		tinyxml2::XMLElement* xmlHeader = xml.NewElement("CAS");
		xml.InsertEndChild(xmlHeader);

		ReadData(buffer, xmlHeader);
		/*
		std::string outfile = WideToUTF8(path) + "_CAS.xml";
		xml.SaveFile(outfile.c_str());
		*/
		tinyxml2::XMLPrinter printer;
		xml.Accept(&printer);
		auto xmlString = std::string{ printer.CStr() };

		std::wcout << UTF8ToWide(xmlString) + L"\n";
		
	}

	//Clear buffers
	buffer.clear();
	file.close();
}

void CAS::ReadData(std::vector<char> buffer, tinyxml2::XMLElement* header)
{
	int position = 0;
	unsigned char seg[4];
	// read header, length is 0x30
	memcpy(&seg, &buffer[0], 4U);
	//if (seg[0] == 0x43 && seg[1] == 0x41 && seg[2] == 0x53 && seg[3] == 0x00)

	// read version
	memcpy(&CAS_Version, &buffer[4], 4U);
	if (CAS_Version == 512)
		header->SetAttribute("version", "41");
	else if (CAS_Version == 515)
		header->SetAttribute("version", "5");

	// read canm offset
	memcpy(&CANM_Offset, &buffer[8], 4U);

	// read t control
	memcpy(&i_TControlCount, &buffer[0x0C], 4U);
	memcpy(&i_TControlOffset, &buffer[0x10], 4U);
	// read v control
	memcpy(&i_VControlCount, &buffer[0x14], 4U);
	memcpy(&i_VControlOffset, &buffer[0x18], 4U);
	// read animation group
	memcpy(&i_AnmGroupCount, &buffer[0x1C], 4U);
	memcpy(&i_AnmGroupOffset, &buffer[0x20], 4U);
	// read bone
	memcpy(&i_BoneCount, &buffer[0x24], 4U);
	memcpy(&i_BoneOffset, &buffer[0x28], 4U);
	// read unk C
	memcpy(&i_UnkCOffset, &buffer[0x2C], 4U);

	// t control data
	ReadTControlData(header, buffer);
	// v control data
	ReadVControlData(header, buffer);
	// animation group data
	ReadAnmGroupData(header, buffer);
	// bone data
	ReadBoneListData(header, buffer);
}

void CAS::ReadTControlData(tinyxml2::XMLElement* header, std::vector<char> buffer)
{
	tinyxml2::XMLElement* xmlTCD = header->InsertNewChildElement("TControl");
	for (int i = 0; i < i_TControlCount; i++)
	{
		int curpos = i_TControlOffset + (i * 0xC);

		int value[3];
		memcpy(&value, &buffer[curpos], 12U);
		tinyxml2::XMLElement* xmlptr = xmlTCD->InsertNewChildElement("ptr");

		// get string
		std::wstring wstr;
		// str
		if (value[0] > 0)
			wstr = ReadUnicode(buffer, curpos + value[0]);
		else
			wstr = L"";
		std::string utf8str = WideToUTF8(wstr);
		xmlptr->SetAttribute("name", utf8str.c_str());

		// read number
		for (int j = 0; j < value[1]; j++)
		{
			int numpos = curpos + value[2] + (j * 4);

			int number;
			memcpy(&number, &buffer[numpos], 4U);

			tinyxml2::XMLElement* xmlNode = xmlptr->InsertNewChildElement("value");
			xmlNode->SetText(number);
		}
	}
	// end
}

void CAS::ReadVControlData(tinyxml2::XMLElement* header, std::vector<char> buffer)
{
	tinyxml2::XMLElement* xmlun = header->InsertNewChildElement("VControl");
	for (int i = 0; i < i_VControlCount; i++)
	{
		int curpos = i_VControlOffset + (i * 0x14);

		int value[3];
		memcpy(&value, &buffer[curpos], 12U);
		tinyxml2::XMLElement* xmlptr = xmlun->InsertNewChildElement("ptr");

		// get string
		std::wstring wstr;
		// str
		if (value[0] > 0)
			wstr = ReadUnicode(buffer, curpos + value[0]);
		else
			wstr = L"";
		std::string utf8str = WideToUTF8(wstr);
		xmlptr->SetAttribute("name", utf8str.c_str());
		//
		tinyxml2::XMLElement* xmlNode = xmlptr->InsertNewChildElement("int");
		xmlNode->SetText(value[1]);
		xmlNode = xmlptr->InsertNewChildElement("int");
		xmlNode->SetText(value[2]);

		float fv;
		memcpy(&fv, &buffer[curpos + 0xC], 4U);
		xmlNode = xmlptr->InsertNewChildElement("float");
		xmlNode->SetText(fv);

		int iv;
		memcpy(&iv, &buffer[curpos + 0x10], 4U);
		xmlNode = xmlptr->InsertNewChildElement("int");
		xmlNode->SetText(iv);
	}
}

void CAS::ReadAnmGroupData(tinyxml2::XMLElement* header, std::vector<char> buffer)
{
	tinyxml2::XMLElement* xmlun = header->InsertNewChildElement("AnmGroup");
	for (int i = 0; i < i_AnmGroupCount; i++)
	{
		int curpos = i_AnmGroupOffset + (i * 0xC);

		int value[3];
		memcpy(&value, &buffer[curpos], 12U);
		tinyxml2::XMLElement* xmlptr = xmlun->InsertNewChildElement("ptr");

		// get string
		std::wstring wstr;
		// str
		if (value[0] > 0)
			wstr = ReadUnicode(buffer, curpos + value[0]);
		else
			wstr = L"";
		std::string utf8str = WideToUTF8(wstr);
		xmlptr->SetAttribute("name", utf8str.c_str());

		for (int j = 0; j < value[1]; j++)
		{
			int ptrpos = curpos + value[2] + (j * 0x24);

			int ptrvalue[9];
			memcpy(&ptrvalue, &buffer[ptrpos], 36U);
			tinyxml2::XMLElement* xmlnode = xmlptr->InsertNewChildElement("node");

			// i0 is string offset
			if (ptrvalue[0] > 0)
				wstr = ReadUnicode(buffer, ptrpos + ptrvalue[0]);
			else
				wstr = L"";
			std::string utf8str = WideToUTF8(wstr);
			xmlnode->SetAttribute("name", utf8str.c_str());
			//
			//
			//
			// undone
		}
	}
}

void CAS::ReadBoneListData(tinyxml2::XMLElement* header, std::vector<char> buffer)
{
	tinyxml2::XMLElement* xmlbone = header->InsertNewChildElement("BoneList");
	for (int i = 0; i < i_BoneCount; i++)
	{
		int curpos = i_BoneOffset + (i * 4);

		int boneofs;
		memcpy(&boneofs, &buffer[curpos], 4U);
		tinyxml2::XMLElement* xmlptr = xmlbone->InsertNewChildElement("value");

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
