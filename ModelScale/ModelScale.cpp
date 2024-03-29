﻿
#include <iostream>
#include <codecvt>
#include "..\EDF_Tools\include\tinyxml2.h"

//Convert UTF8 to wide string
std::wstring UTF8ToWide(const std::string& source)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	return conv.from_bytes(source);
}

std::string WideToUTF8(const std::wstring& source)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	return conv.to_bytes(source);
}

void __fastcall NewFunction(tinyxml2::XMLElement* data, float scaleSize)
{
	float vf;
	vf = data->FloatAttribute("ix");
	vf *= scaleSize;
	data->SetAttribute("ix", vf);

	vf = data->FloatAttribute("iy");
	vf *= scaleSize;
	data->SetAttribute("iy", vf);

	vf = data->FloatAttribute("iz");
	vf *= scaleSize;
	data->SetAttribute("iz", vf);
	//
	vf = data->FloatAttribute("vx");
	vf *= scaleSize;
	data->SetAttribute("vx", vf);

	vf = data->FloatAttribute("vy");
	vf *= scaleSize;
	data->SetAttribute("vy", vf);

	vf = data->FloatAttribute("vz");
	vf *= scaleSize;
	data->SetAttribute("vz", vf);
}

void NewFunction2(tinyxml2::XMLElement* data, float vf)
{
	data->SetAttribute("type", "0");
	data->SetAttribute("frame", "1");
	data->SetAttribute("ix", vf);
	data->SetAttribute("iy", vf);
	data->SetAttribute("iz", vf);
	data->SetAttribute("vx", "0");
	data->SetAttribute("vy", "0");
	data->SetAttribute("vz", "0");
}

void CheckXMLHeader(std::wstring path, float scaleSize) {
	std::string UTF8Path = WideToUTF8(path);

	tinyxml2::XMLDocument doc;
	doc.LoadFile(UTF8Path.c_str());

	tinyxml2::XMLNode* header = doc.FirstChildElement("CAS");
	tinyxml2::XMLElement* Data = header->FirstChildElement("CanmData");

	tinyxml2::XMLElement* entry, * entry2, *entry3, *entry4;
	std::string str, type;
	std::wcout << L"Read CANM animation list:\n";
	entry = Data->FirstChildElement("AnmData");
	for (entry2 = entry->FirstChildElement("node"); entry2 != 0; entry2 = entry2->NextSiblingElement("node"))
	{
		for (entry3 = entry2->FirstChildElement("value"); entry3 != 0; entry3 = entry3->NextSiblingElement("value"))
		{
			
			str = entry3->Attribute("bone");
			/*
			if (str != "Scene_Root" && str != "globalSRT") {
				entry4 = entry3->FirstChildElement("position");
				type = entry4->Attribute("type");
				if (type != "null")
				{
					NewFunction(entry4, scaleSize);
				}
			}*/

			if (str == "kosi"){
				entry4 = entry3->FirstChildElement("scaling");
				type = entry4->Attribute("type");
				if (type == "null")
				{
					NewFunction2(entry4, scaleSize);
				}
				else {
					NewFunction(entry4, scaleSize);
				}
				break;
			}
			//
		}
		/*
		if (HASmdl == 0) {
			tinyxml2::XMLElement* xmlptr = entry2->InsertNewChildElement("value");
			xmlptr->SetAttribute("bone", "mdl");

			tinyxml2::XMLElement* xmlPos = xmlptr->InsertNewChildElement("position");
			NewFunction2(xmlPos, 0.0f);

			tinyxml2::XMLElement* xmlRot = xmlptr->InsertNewChildElement("rotation");
			NewFunction2(xmlRot, 0.0f);

			tinyxml2::XMLElement* xmlVis = xmlptr->InsertNewChildElement("scaling");
			NewFunction2(xmlVis, scaleSize);
		}*/
	}

	doc.SaveFile(UTF8Path.c_str());
}

int wmain(int argc, wchar_t* argv[])
{
    using namespace std;

    wstring path;

	wcout << L"Filename:";
	wcin >> path;
	wcout << L"\n";

	wcout << L"Parsing file...\n";

	CheckXMLHeader(path, 2.0f);

	system("pause");
}
