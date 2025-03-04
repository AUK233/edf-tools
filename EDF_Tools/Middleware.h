#pragma once
#include "include/tinyxml2.h"

// Check for the extra file header
void CheckDataType(const std::vector<char>& buffer, tinyxml2::XMLElement*& xmlHeader, const std::string& str);
// write
// Check the header to determine the output type
void CheckXMLHeader(const std::wstring& path);
// Check for the extra file header when writing
std::vector< char > CheckDataType(tinyxml2::XMLElement* Data, tinyxml2::XMLNode* header);