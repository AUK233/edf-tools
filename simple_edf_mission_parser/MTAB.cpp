#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <algorithm>

#include "Middleware.h"
#include "util.h"
#include "MTAB.h"
#include "include/tinyxml2.h"

void MTAB::Read(std::wstring path)
{
	std::ifstream file(path + L".mtab", std::ios::binary | std::ios::ate | std::ios::in);

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
		xmlMain->SetAttribute("header", "MTAB");

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

void MTAB::ReadData(std::vector<char> buffer, tinyxml2::XMLElement* header, tinyxml2::XMLElement* xmlHeader)
{
	int position = 0;
	unsigned char seg[4];
	// read header, length is 0x24
	memcpy(&seg, &buffer[0x10], 4U);
	//if (seg[0] == 0x4D && seg[1] == 0x54 && seg[2] == 0x41 && seg[3] == 0x42)

	// get header data
	int i_hd1;
	float f_hd2;
	memcpy(&i_hd1, &buffer[0], 4U);
	header->SetAttribute("int1", i_hd1);
	memcpy(&f_hd2, &buffer[4], 4U);
	header->SetAttribute("time", f_hd2);
	// now this value is not needed
	//memcpy(&i_hd3, &buffer[8], 4U);
	//header->SetAttribute("int3", i_hd3);

	// get main actions
	memcpy(&i_MainActionCount, &buffer[0xC], 4U);
	memcpy(&i_MainActionOffset, &buffer[0x18], 4U);

	// read main action
	position = i_MainActionOffset;
	for (int i = 0; i < i_MainActionCount; i++)
	{
		int curpos = position + (i * 0xC);

		tinyxml2::XMLElement* xmlMA = header->InsertNewChildElement("group");
		ReadMainActionData(buffer, curpos, xmlMA, xmlHeader);
	}
}

void MTAB::ReadMainActionData(std::vector<char>& buffer, int curpos, tinyxml2::XMLElement* xmlData, tinyxml2::XMLElement* xmlHeader)
{

#if defined(DEBUGMODE)
	xmlData->SetAttribute("pos", curpos);
#endif

	int value[3];
	memcpy(&value, &buffer[curpos], 12U);
	// get wide string
	std::wstring wstr;
	if (value[1] > 0)
		wstr = ReadUnicode(buffer, curpos + value[1]);
	else
		wstr = L"";
	xmlData->SetAttribute("name", WideToUTF8(wstr).c_str());
	// get sub action
	for (int i = 0; i < value[0]; i++)
	{
		int curofs = curpos + value[2] + (i * 0xC);

		tinyxml2::XMLElement* xmlSA = xmlData->InsertNewChildElement("action");
		ReadSubActionData(buffer, curofs, xmlSA, xmlHeader);
	}
}

void MTAB::ReadSubActionData(std::vector<char>& buffer, int curpos, tinyxml2::XMLElement* xmlData, tinyxml2::XMLElement* xmlHeader)
{

#if defined(DEBUGMODE)
	xmlData->SetAttribute("pos", curpos);
#endif

	int value[3];
	memcpy(&value, &buffer[curpos], 12U);
	// get string
	std::string str;
	if (value[1] > 0)
		str = ReadASCII(buffer, curpos + value[1]);
	else
		str = "";
	xmlData->SetAttribute("name", str.c_str());
	// get sub action
	for (int i = 0; i < value[0]; i++)
	{
		int curofs = curpos + value[2] + (i * 0x14);

		tinyxml2::XMLElement* xmlNode = xmlData->InsertNewChildElement("node");
		ReadNodeData(buffer, curofs, xmlNode, xmlHeader);
	}
}

void MTAB::ReadNodeData(std::vector<char>& buffer, int curpos, tinyxml2::XMLElement* xmlData, tinyxml2::XMLElement* xmlHeader)
{

#if defined(DEBUGMODE)
	xmlData->SetAttribute("pos", curpos);
#endif

	int value[5];
	memcpy(&value, &buffer[curpos], 20U);
	xmlData->SetAttribute("int1", value[0]);
	xmlData->SetAttribute("int3", value[2]);
	xmlData->SetAttribute("parameter", value[3]);

	// get float
	for (int i = 0; i < value[1]; i++)
	{
		int ptrofs = curpos + value[4] + (i * 0x40);

		tinyxml2::XMLElement* xmlPtr = xmlData->InsertNewChildElement("ptr");
		for (int j = 0; j < 4; j++)
		{
			int curofs = ptrofs + (j * 0x10);
			tinyxml2::XMLElement* xmlNode = xmlPtr->InsertNewChildElement("float");
#if defined(DEBUGMODE)
			xmlNode->SetAttribute("pos", curofs);
#endif
			float vf[4];
			memcpy(&vf, &buffer[curofs], 16U);
			xmlNode->SetAttribute("timing", vf[0]);
			xmlNode->SetAttribute("x", vf[1]);
			xmlNode->SetAttribute("y", vf[2]);
			xmlNode->SetAttribute("z", vf[3]);
		}
	}
}

void MTAB::Write(std::wstring path, tinyxml2::XMLNode* header)
{
	std::wcout << "Will output MTAB file.\n";

	tinyxml2::XMLElement* mainData = header->FirstChildElement("Main");

	std::vector< char > bytes;
	bytes = WriteData(mainData, header);

	//Final write.
	/**/
	std::ofstream newFile(path + L".mtab", std::ios::binary | std::ios::out | std::ios::ate);

	for (int i = 0; i < bytes.size(); i++)
	{
		newFile << bytes[i];
	}

	newFile.close();

	std::wcout << L"Conversion completed: " + path + L".mtab\n";
}

std::vector<char> MTAB::WriteData(tinyxml2::XMLElement* mainData, tinyxml2::XMLNode* header)
{
	std::vector< char > bytes;

	// read header
	int headerInt = mainData->IntAttribute("int1");
	float headerTime = mainData->FloatAttribute("time");

	// get main action
	for (tinyxml2::XMLElement* entry = mainData->FirstChildElement("group"); entry != 0; entry = entry->NextSiblingElement("group"))
	{
		v_MainAction.push_back(WriteMainAction(entry));
	}
	i_MainActionCount = v_MainAction.size();
	i_SubActionCount = v_Data.size();

	// sort string
	std::sort(NameList.begin(), NameList.end());
	std::sort(WNameList.begin(), WNameList.end());

	// push bytes!
	bytes.resize(0x1C, 0);
	bytes[0x10] = 0x4D;
	bytes[0x11] = 0x54;
	bytes[0x12] = 0x41;
	bytes[0x13] = 0x42;
	bytes[0x15] = 0x02;
	bytes[0x18] = 0x1C;
	// copy remain
	memcpy(&bytes[0], &headerInt, 4U);
	memcpy(&bytes[4], &headerTime, 4U);
	memcpy(&bytes[8], &i_SubActionCount, 4U);
	memcpy(&bytes[0xC], &i_MainActionCount, 4U);

	// write main action
	for (size_t i = 0; i < v_MainAction.size(); i++)
	{
		v_MainAction[i].pos = bytes.size();
		for (size_t j = 0; j < v_MainAction[i].bytes.size(); j++)
			bytes.push_back(v_MainAction[i].bytes[j]);
	}
	// write sub action
	for (size_t i = 0; i < v_SubAction.size(); i++)
	{
		int safpos = bytes.size();
		// write offset to main action
		for (size_t j = 0; j < v_MainAction.size(); j++)
		{
			if (v_MainAction[j].saofs == i)
			{
				int mapos = v_MainAction[j].pos;
				int safofs = safpos - mapos;
				memcpy(&bytes[mapos + 8], &safofs, 4U);
			}
		}

		v_SubAction[i].pos = safpos;
		for (size_t j = 0; j < v_SubAction[i].bytes.size(); j++)
			bytes.push_back(v_SubAction[i].bytes[j]);
	}

	// write data 1
	for (size_t i = 0; i < v_Data.size(); i++)
	{
		int d1_pos = bytes.size();
		// write offset to sub action
		for (size_t j = 0; j < v_SubAction.size(); j++)
		{
			if (v_SubAction[j].saofs == i)
			{
				int sapos = v_SubAction[j].pos;
				int d1_ofs = d1_pos - sapos;
				memcpy(&bytes[sapos + 8], &d1_ofs, 4U);
			}
		}

		v_Data[i].pos = d1_pos;
		for (size_t j = 0; j < v_Data[i].bytes1.size(); j++)
			bytes.push_back(v_Data[i].bytes1[j]);
	}
	// 16-byte alignment is required
	int i_Alignment = bytes.size() % 16;
	if (i_Alignment > 0)
	{
		for (int i = i_Alignment; i < 16; i++)
			bytes.push_back(0);
	}
	// write data 2
	for (size_t i = 0; i < v_Data.size(); i++)
	{
		int d2_pos = bytes.size();
		// write offset to data
		int d1_pos = v_Data[i].pos;
		int d2_ofs = d2_pos - d1_pos;
		memcpy(&bytes[d1_pos + 0x10], &d2_ofs, 4U);

		for (size_t j = 0; j < v_Data[i].bytes2.size(); j++)
			bytes.push_back(v_Data[i].bytes2[j]);
	}

	// write string
	for (size_t i = 0; i < NameList.size(); i++)
	{
		int strpos = bytes.size();
		PushStringToVector(NameList[i], &bytes);
		//write string offset
		for (size_t j = 0; j < v_SubAction.size(); j++)
		{
			if (v_SubAction[j].str == NameList[i])
			{
				int strofs = strpos - v_SubAction[j].pos;
				memcpy(&bytes[v_SubAction[j].pos + 4], &strofs, 4U);
			}
		}
	}
	// write wide string
	for (size_t i = 0; i < WNameList.size(); i++)
	{
		int strpos = bytes.size();
		PushWStringToVector(WNameList[i], &bytes);
		//write string offset
		for (size_t j = 0; j < v_MainAction.size(); j++)
		{
			if (v_MainAction[j].wstr == WNameList[i])
			{
				int strofs = strpos - v_MainAction[j].pos;
				memcpy(&bytes[v_MainAction[j].pos + 4], &strofs, 4U);
			}
		}
	}

	return bytes;
}

MTABMainAction MTAB::WriteMainAction(tinyxml2::XMLElement* data)
{
	MTABMainAction out;
	// get string
	std::wstring wstr = UTF8ToWide(data->Attribute("name"));
	// check for duplication
	bool isExist = false;
	for (size_t i = 0; i < WNameList.size(); i++)
	{
		if (WNameList[i] == wstr)
		{
			isExist = true;
			break;
		}
	}

	if (!isExist)
	{
		WNameList.push_back(wstr);
	}
	out.wstr = wstr;

	// get sub action
	out.saofs = v_SubAction.size();
	int count = 0;
	for (tinyxml2::XMLElement* entry = data->FirstChildElement("action"); entry != 0; entry = entry->NextSiblingElement("action"))
	{
		v_SubAction.push_back(WriteSubAction(entry));
		count++;
	}

	out.bytes.resize(0xC, 0);

	memcpy(&out.bytes[0], &count, 4U);

	return out;
}

MTABMainAction MTAB::WriteSubAction(tinyxml2::XMLElement* data)
{
	MTABMainAction out;
	// get string
	std::string str = data->Attribute("name");
	// check for duplication
	bool isExist = false;
	for (size_t i = 0; i < NameList.size(); i++)
	{
		if (NameList[i] == str)
		{
			isExist = true;
			break;
		}
	}

	if (!isExist)
	{
		NameList.push_back(str);
	}
	out.str = str;

	// get data node
	out.saofs = v_Data.size();
	int count = 0;
	for (tinyxml2::XMLElement* entry = data->FirstChildElement("node"); entry != 0; entry = entry->NextSiblingElement("node"))
	{
		v_Data.push_back(WriteDataNode(entry));
		count++;
	}

	out.bytes.resize(0xC, 0);

	memcpy(&out.bytes[0], &count, 4U);

	return out;
}

MTABData MTAB::WriteDataNode(tinyxml2::XMLElement* data)
{
	MTABData out;

	int value[4];
	// it always 16
	value[2] = 16;
	value[0] = data->IntAttribute("int1");
	value[3] = data->IntAttribute("parameter");

	int count = 0;
	for (tinyxml2::XMLElement* entry = data->FirstChildElement("ptr"); entry != 0; entry = entry->NextSiblingElement("ptr"))
	{
		// 4 x float
		char bytes[16];
		float vf[4];

		for (tinyxml2::XMLElement* entry2 = entry->FirstChildElement("float"); entry2 != 0; entry2 = entry2->NextSiblingElement("float"))
		{
			vf[0] = entry2->FloatAttribute("timing");
			vf[1] = entry2->FloatAttribute("x");
			vf[2] = entry2->FloatAttribute("y");
			vf[3] = entry2->FloatAttribute("z");

			memcpy(&bytes, &vf, 16U);

			for (size_t i = 0; i < 16; i++)
				out.bytes2.push_back(bytes[i]);
		}

		count++;
	}
	value[1] = count;

	out.bytes1.resize(0x14, 0);
	memcpy(&out.bytes1[0], &value, 16U);

	return out;
}
