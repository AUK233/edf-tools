#include <iostream>
#include "..\EDF_Tools\include\tinyxml2.h"

void DSGOsubtitle2SGO(const std::wstring& path) {
	FILE* INFile = 0;
	_wfopen_s(&INFile, path.c_str(), L"rb");
	tinyxml2::XMLDocument INdoc;
	INdoc.LoadFile(INFile);

	tinyxml2::XMLNode* INheader = INdoc.FirstChildElement("EDFDATA");
	tinyxml2::XMLElement* INmain = INheader->FirstChildElement("Main");
	std::string headerType = INmain->Attribute("header");
	if (headerType != "DSGO") {
		std::wcout << L"wrong file input!\n";
		system("pause");
		return;
	}

	tinyxml2::XMLElement* INentry = INmain->FirstChildElement();
	headerType = INentry->Attribute("name");
	if (headerType != "table") {
		std::wcout << L"wrong DSGO file input!\n";
		system("pause");
		return;
	}

	// start output file
	tinyxml2::XMLDocument xml;
	xml.InsertFirstChild(xml.NewDeclaration());
	tinyxml2::XMLElement* xmlHeader = xml.NewElement("EDFDATA");
	xml.InsertEndChild(xmlHeader);
	tinyxml2::XMLElement* xmlMain = xmlHeader->InsertNewChildElement("Main");
	xmlMain->SetAttribute("header", "SGO");
	// 
	tinyxml2::XMLElement* xmlNode, *entry2, *entry3;
	std::string nameStr, textStr, nameStr2;

	for (entry2 = INentry->FirstChildElement(); entry2 != 0; entry2 = entry2->NextSiblingElement()) {
		nameStr = entry2->Attribute("name");
		// get subtitle text
		for (entry3 = entry2->FirstChildElement(); entry3 != 0; entry3 = entry3->NextSiblingElement()) {
			headerType = entry3->Attribute("name");
			if (headerType == "caption") {
				textStr = entry3->GetText();
				break;
			}
		}
		// output subtitle
		xmlNode = xmlMain->InsertNewChildElement("string");
		xmlNode->SetAttribute("name", nameStr.c_str());
		xmlNode->SetText(textStr.c_str());

		nameStr2 = nameStr + "_E";
		xmlNode = xmlMain->InsertNewChildElement("string");
		xmlNode->SetAttribute("name", nameStr2.c_str());
		xmlNode->SetText(textStr.c_str());

		nameStr2 = nameStr + "_S";
		xmlNode = xmlMain->InsertNewChildElement("string");
		xmlNode->SetAttribute("name", nameStr2.c_str());
		xmlNode->SetText(textStr.c_str());
	}
	// ※ notice this character.
	xml.SaveFile("output_DATA.xml");
}

int wmain(int argc, wchar_t* argv[])
{
	using namespace std;

	wstring path;
	if (argc > 1) {
		path = argv[1];
	}
	else {
		wcout << L"Filename: ";
		wcin >> path;
		wcout << L"\n";
	}
	DSGOsubtitle2SGO(path);
}
