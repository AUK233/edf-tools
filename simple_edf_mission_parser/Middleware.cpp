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
#include "SGO.h"
#include "MAB.h"
#include "include/tinyxml2.h"

void CheckDataType(std::vector<char>& buffer, tinyxml2::XMLElement*& xmlHeader, std::string str)
{
	char header[4];
	header[0] = buffer[0];
	header[1] = buffer[1];
	header[2] = buffer[2];
	header[3] = buffer[3];

	tinyxml2::XMLElement* NewXml = xmlHeader->InsertNewChildElement("Subdata");
	NewXml->SetAttribute("name", str.c_str());

	if ((header[0] == 0x53 && header[1] == 0x47 && header[2] == 0x4f && header[3] == 0x00) || (header[3] == 0x53 && header[2] == 0x47 && header[1] == 0x4f && header[0] == 0x00))
	{
		NewXml->SetAttribute("header", "SGO");

		std::unique_ptr< SGO > sgoReader = std::make_unique< SGO >();
		sgoReader->ReadData(buffer, NewXml, xmlHeader);
		sgoReader.reset();
	}
	else if (header[0] == 0x4D && header[1] == 0x41 && header[2] == 0x42 && header[3] == 0x00)
	{
		NewXml->SetAttribute("header", "MAB");

		std::unique_ptr< MAB > mabReader = std::make_unique< MAB >();
		mabReader->ReadData(buffer, NewXml, xmlHeader);
		mabReader.reset();
	}
	else
	{
		NewXml->SetAttribute("header", "RAW");

		std::string rawstr = ReadRaw(buffer, 0, buffer.size());
		NewXml->SetText(rawstr.c_str());
	}
}

// Check the header to determine the output type
void CheckXMLHeader(std::wstring path)
{
	std::wstring sourcePath = path + L"_data.xml";
	std::string UTF8Path = WideToUTF8(sourcePath);

	tinyxml2::XMLDocument doc;
	doc.LoadFile(UTF8Path.c_str());

	tinyxml2::XMLNode* header = doc.FirstChildElement("EDFDATA");
	tinyxml2::XMLElement* mainData = header->FirstChildElement("Main");

	std::string headerType = mainData->Attribute("header");
	if (headerType == "SGO")
	{
		std::unique_ptr< SGO > writer = std::make_unique< SGO >();
		writer->Write(path, header);
		writer.reset();
	}
	else if (headerType == "MAB")
	{
		std::unique_ptr< MAB > writer = std::make_unique< MAB >();
		writer->Write(path, header);
		writer.reset();
	}
}

// Check for the extra file header
std::vector<char> CheckDataType(tinyxml2::XMLElement* Data, tinyxml2::XMLNode* header)
{
	std::vector<char> bytes;

	std::string headerType = Data->Attribute("header");
	if (headerType == "SGO")
	{
		std::unique_ptr< SGO > writer = std::make_unique< SGO >();
		bytes = writer->WriteData(Data, header);
		writer.reset();
	}
	else if (headerType == "MAB")
	{
		std::unique_ptr< MAB > writer = std::make_unique< MAB >();
		bytes = writer->WriteData(Data, header);
		writer.reset();
	}
	else if (headerType == "RAW")
	{
		std::string argsStrn = Data->GetText();
		if (argsStrn.length() % 2 > 0)
		{
			argsStrn = ("0" + argsStrn);
		}
		int count = 0;
		//Convert to hex.
		char byte;
		for (unsigned int i = 0; i < argsStrn.length(); i += 2)
		{
			std::string byteString = argsStrn.substr(i, 2);
			byte = (char)std::stol(byteString.c_str(), NULL, 16);
			bytes.push_back(byte);
			count++;
		}
	}

	//std::wcout << L"\ndata size: " + ToString(int(bytes.size())) + L"\n\n";

	return bytes;
}
