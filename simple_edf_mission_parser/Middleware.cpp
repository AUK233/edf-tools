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
#include "include/tinyxml2.h"

void CheckDataType(std::vector<char>& buffer, tinyxml2::XMLElement*& xmlNode, tinyxml2::XMLElement*& xmlHeader, int namepos)
{
	char header[4];
	header[0] = buffer[0];
	header[1] = buffer[1];
	header[2] = buffer[2];
	header[3] = buffer[3];

	tinyxml2::XMLElement* NewXml = xmlHeader->InsertNewChildElement("subdata");
	NewXml->SetAttribute("name", namepos);

	if ((header[0] == 0x53 && header[1] == 0x47 && header[2] == 0x4f && header[3] == 0x00) || (header[3] == 0x53 && header[2] == 0x47 && header[1] == 0x4f && header[0] == 0x00))
	{
		NewXml->SetAttribute("header", "SGO");

		std::unique_ptr< SGO > sgoReader = std::make_unique< SGO >();
		sgoReader->ReadData(buffer, NewXml, xmlHeader);
		sgoReader.reset();
	}
	else
	{
		NewXml->SetAttribute("header", "RAW");

		std::string rawstr = ReadRaw(buffer, 0, buffer.size());
		NewXml->SetText(rawstr.c_str());
	}
}