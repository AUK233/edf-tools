#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>


#include "clAWB.h"
#include "clUtil.h"
#include "clACB.h"

void ACB::Read(const std::string& inPath)
{
	// read acb
	InPath = inPath;
	std::string tempPath = inPath + ".acb";
	if (!std::filesystem::exists(tempPath)) {
		std::cout << "There is no ACB file!\n";
		return;
	}
	std::ifstream acb_file(tempPath, std::ios::binary | std::ios::ate | std::ios::in);
	std::streamsize acb_size = acb_file.tellg();
	acb_file.seekg(0, std::ios::beg);
	std::vector<char> ACB_Buffer(acb_size);

	int ReadFileState = 0;
	if (acb_file.read(ACB_Buffer.data(), acb_size)) {
		ReadFileState = 1;
	}
	acb_file.close();

	if (!ReadFileState) {
		return;
	}

	// ======================================

	// ACB, check header
	// @UTF
	if (*(UINT32*)&ACB_Buffer[0] != 1179931968) {
		std::cout << "This is not an ACB file!\n";
		return;
	}
	
	// create xml
	tinyxml2::XMLDocument xml;
	xml.InsertFirstChild(xml.NewDeclaration());
	tinyxml2::XMLElement* xmlHeader = xml.NewElement("ACB");
	xml.InsertEndChild(xmlHeader);

	// read data
	ReadUTFData(ACB_Buffer, xmlHeader);

	// write file
	std::string outfile = inPath + ".xml";
	xml.SaveFile(outfile.c_str());
}

void ACB::ReadUTFData(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader)
{
	// get header
	ReadUTFHeaderData(buffer);
	// get schema
	int headerSchemaSize = v_header.NodeDataOffset - 0x20;
	if (headerSchemaSize >= 0x8000) {
		std::cout << "Schema is too large!\n";
		return;
	}

	// read only once per utf
	ReadUTFParametersList(buffer, v_header.ParameterCount);

	//std::vector<std::string> v_header_names;
	//ReadGetStringList(ACB_Buffer, v_header.NameTableOffset, v_header.DataOffset, v_header_names);
	for (int i = 0; i < v_header.NodeCount; i++) {
		tinyxml2::XMLElement* xmldata = xmlHeader->InsertNewChildElement("node");
		for (int j = 0; j < v_parameters.size(); j++) {
			UTF_GetParameters inPtr;
			int isBigEndian = -1;
			if (v_parameters[j].flag & COLUMN_FLAG_DEFAULT) {
				isBigEndian = 0;
				inPtr.xmlNode = xmldata;
				inPtr.index = j;
				inPtr.isNodePtr = 0;
				inPtr.PtrDataOfs = v_parameters[j].offset;
			}
			else if (v_parameters[j].flag & COLUMN_FLAG_ROW) {
				isBigEndian = 1;
				inPtr.xmlNode = xmldata;
				inPtr.index = j;
				inPtr.isNodePtr = 1;
				inPtr.PtrDataOfs = v_parameters[j].offset + (i * v_header.NodeDataSize);
			}
			else {
				inPtr.xmlNode = 0;
			}

			if (inPtr.xmlNode) {
				int dataType = v_parameters[j].type;
				switch (dataType) {
				case COLUMN_TYPE_UINT8:
				case COLUMN_TYPE_SINT8:
				case COLUMN_TYPE_UINT16:
				case COLUMN_TYPE_SINT16:
				case COLUMN_TYPE_UINT32:
				case COLUMN_TYPE_SINT32:
				case COLUMN_TYPE_UINT64:
				case COLUMN_TYPE_SINT64:
					ReadUTFParameter_Int(buffer, inPtr, dataType);
					break;
				case COLUMN_TYPE_FLOAT:
					ReadUTFParameter_FP32(buffer, inPtr);
					break;
				case COLUMN_TYPE_STRING:
					ReadUTFParameter_String(buffer, inPtr);
					break;
				case COLUMN_TYPE_VLDATA:
					ReadUTFParameter_ToData(buffer, inPtr);
					break;
				default:
					break;
				}
			}
			// end 1
		}
		// end 2
	}
	
}

void ACB::ReadUTFHeaderData(const std::vector<char>& buffer)
{
	v_header.U32_header = ReadUINT32(&buffer[0], 0);
	v_header.BlockSize = ReadUINT32(&buffer[4], 1) + 8;
	v_header.Version = ReadUINT16(&buffer[8], 1);
	v_header.NodeDataOffset = ReadUINT16(&buffer[0xA], 1) + 8;
	v_header.NameTableOffset = ReadUINT32(&buffer[0xC], 1) + 8;
	v_header.DataOffset = ReadUINT32(&buffer[0x10], 1) + 8;
	v_header.NameOffset = ReadUINT32(&buffer[0x14], 1);
	v_header.ParameterCount = ReadUINT16(&buffer[0x18], 1);
	v_header.NodeDataSize = ReadUINT16(&buffer[0x1A], 1);
	v_header.NodeCount = ReadUINT32(&buffer[0x1C], 1);
}

void ACB::ReadGetStringList(const std::vector<char>& in, int inStart, int inEnd, std::vector<std::string>& out)
{
	// get string list
	int offset = inStart;
	for (int i = inStart; i < inEnd; i++) {
		if (in[i] == 0) {
			out.push_back(EscapeControlChars(std::string(&in[offset], i - offset)));
			offset = i + 1;
			if (offset < inEnd) {
				if (in[offset] == 0) {
					return;
				}
			}
		}
	}
}

void ACB::ReadUTFParametersList(const std::vector<char>& in, int inSize)
{
	UINT32 value_size, column_offset = 0;
	int curpos = 0x20;
	// get column data
	for (int i = 0; i < inSize; i++) {
		UINT8 info = (UINT8)in[curpos];
		int nameOffset = ReadUINT32(&in[curpos + 1], 1);

		curpos += 5;

		UTF_Column_t out;
		out.flag = info & 0xF0;
		out.type = info & 0x0F;
		out.name = 0;
		out.offset = -1;

		switch (out.type) {
		case COLUMN_TYPE_UINT8:
		case COLUMN_TYPE_SINT8:
			value_size = 0x01;
			break;
		case COLUMN_TYPE_UINT16:
		case COLUMN_TYPE_SINT16:
			value_size = 0x02;
			break;
		case COLUMN_TYPE_UINT32:
		case COLUMN_TYPE_SINT32:
		case COLUMN_TYPE_FLOAT:
		case COLUMN_TYPE_STRING:
			value_size = 0x04;
			break;
		case COLUMN_TYPE_UINT64:
		case COLUMN_TYPE_SINT64:
		//case COLUMN_TYPE_DOUBLE:
		case COLUMN_TYPE_VLDATA:
			value_size = 0x08;
			break;
		//case COLUMN_TYPE_UINT128:
		//    value_size = 0x16;
		default:
			break;
		}

		if (out.flag & COLUMN_FLAG_NAME) {
			out.name = &in[v_header.NameTableOffset + nameOffset];
		}
		else {
			out.name = "";
		}

		if (out.flag & COLUMN_FLAG_DEFAULT) {
			//Swap4Bytes(const_cast<char*>(&in[curpos]));
			
			// this has a problem
			out.offset = curpos;
			curpos += value_size;
		}

		if (out.flag & COLUMN_FLAG_ROW) {
			out.offset = column_offset + v_header.NodeDataOffset;
			column_offset += value_size;
		}

		v_parameters.push_back(out);
	}
}

void ACB::ReadUTFParameter_Int(const std::vector<char>& in, const UTF_GetParameters& inPtr, int dataType)
{
	int curpos = inPtr.PtrDataOfs;
	//int isBigEndian = inPtr.isNodePtr;
	int isBigEndian = 1;
	tinyxml2::XMLElement* xmlNode = inPtr.xmlNode;
	const UTF_Column_t& data = v_parameters[inPtr.index];

	int wordSize;
	INT64 s64;
	UINT64 u64;

	switch (dataType) {
	case COLUMN_TYPE_UINT8:
		u64 = (UINT8)in[curpos];
		wordSize = 1;
		break;
	case COLUMN_TYPE_SINT8:
		s64 = (INT8)in[curpos];
		wordSize = -1;
		break;
	case COLUMN_TYPE_UINT16:
		u64 = ReadUINT16(&in[curpos], isBigEndian);
		wordSize = 2;
		break;
	case COLUMN_TYPE_SINT16:
		s64 = ReadUINT16(&in[curpos], isBigEndian);
		wordSize = -2;
		break;
	case COLUMN_TYPE_UINT32:
		u64 = ReadUINT32(&in[curpos], isBigEndian);
		wordSize = 4;
		break;
	case COLUMN_TYPE_SINT32:
		s64 = ReadUINT32(&in[curpos], isBigEndian);
		wordSize = -4;
		break;
	case COLUMN_TYPE_UINT64:
		u64 = ReadUINT64(&in[curpos], isBigEndian);
		wordSize = 8;
		break;
	case COLUMN_TYPE_SINT64:
		s64 = ReadUINT64(&in[curpos], isBigEndian);
		wordSize = -8;
		break;
	default:
		return;
	}

	tinyxml2::XMLElement* xmlptr = xmlNode->InsertNewChildElement("int");
	xmlptr->SetAttribute("name", data.name);
	xmlptr->SetAttribute("variable", isBigEndian);
	if (wordSize > 0) {
		xmlptr->SetAttribute("size", wordSize);
		xmlptr->SetAttribute("sign", "0");
		xmlptr->SetAttribute("value", u64);
	}
	else {
		xmlptr->SetAttribute("size", -wordSize);
		xmlptr->SetAttribute("sign", "1");
		xmlptr->SetAttribute("value", s64);
	}
}

void ACB::ReadUTFParameter_FP32(const std::vector<char>& in, const UTF_GetParameters& inPtr)
{
	int curpos = inPtr.PtrDataOfs;
	int isBigEndian = inPtr.isNodePtr;
	tinyxml2::XMLElement* xmlNode = inPtr.xmlNode;
	const UTF_Column_t& data = v_parameters[inPtr.index];

	UINT32 u32 = ReadUINT32(&in[curpos], 1);
	float f32 = *(float*)&u32;

	tinyxml2::XMLElement* xmlptr = xmlNode->InsertNewChildElement("float");
	xmlptr->SetAttribute("name", data.name);
	xmlptr->SetAttribute("value", f32);
}

void ACB::ReadUTFParameter_String(const std::vector<char>& in, const UTF_GetParameters& inPtr)
{
	int curpos = inPtr.PtrDataOfs;
	int isBigEndian = inPtr.isNodePtr;
	tinyxml2::XMLElement* xmlNode = inPtr.xmlNode;
	const UTF_Column_t& data = v_parameters[inPtr.index];

	curpos = ReadUINT32(&in[curpos], 1) + v_header.NameTableOffset;
	std::string str = EscapeControlChars(std::string(&in[curpos]));

	tinyxml2::XMLElement* xmlptr = xmlNode->InsertNewChildElement("string");
	xmlptr->SetAttribute("name", data.name);
	xmlptr->SetAttribute("value", str.c_str());
}

void ACB::ReadUTFParameter_ToData(const std::vector<char>& in, const UTF_GetParameters& inPtr)
{
	int curpos = inPtr.PtrDataOfs;
	int isBigEndian = inPtr.isNodePtr;
	tinyxml2::XMLElement* xmlNode = inPtr.xmlNode;
	const UTF_Column_t& data = v_parameters[inPtr.index];

	tinyxml2::XMLElement* xmlptr = xmlNode->InsertNewChildElement("data");
	xmlptr->SetAttribute("name", data.name);

	int dataPos = ReadUINT32(&in[curpos], 1) + v_header.DataOffset;
	int dataSize = ReadUINT32(&in[curpos+4], 1);

	xmlptr->SetAttribute("flag", data.flag);

	if (!dataSize) {
		//xmlptr->SetAttribute("debug", ReadUINT32(&in[curpos], isBigEndian));
		xmlptr->SetAttribute("type", "NULL");
		return;
	}

	if (dataPos >= v_header.BlockSize) {
		xmlptr->SetAttribute("debug", curpos);
		xmlptr->SetAttribute("type", "NULL");
		return;
	}

	if (*(UINT32*)&in[dataPos] == 1179931968) {
		xmlptr->SetAttribute("type", "UTF");

		std::vector<char> newbuf(in.begin() + dataPos, in.begin() + dataPos + dataSize);

		std::unique_ptr< ACB > pACB = std::make_unique< ACB >();
		pACB->InPath = InPath;
		pACB->ReadUTFData(newbuf, xmlptr);
		pACB.reset();
	}
	else {
		xmlptr->SetAttribute("type", "RAW");
		std::string rawData = RawDataToHexString(&in[dataPos], dataSize);
		xmlptr->SetText(rawData.c_str());
	}
}
