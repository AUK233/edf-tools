#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <format>

#include "clAWB.h"
#include "clUtil.h"
#include "clACB.h"

// 0xDDDDDDDD is +4, 0xCCCCCCCC is +29h, BE
static const char RAW_StreamAwbAfs2Header[] = {
	0x40, 0x55, 0x54, 0x46, 0xDD, 0xDD, 0xDD, 0xDD, 0x00, 0x01, 0x00, 0x1D, 0x00, 0x00, 0x00, 0x25,
	0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01,
	0x5B, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0x53, 0x74, 0x72,
	0x65, 0x61, 0x6D, 0x41, 0x77, 0x62, 0x48, 0x65, 0x61, 0x64, 0x65, 0x72, 0x00, 0x48, 0x65, 0x61,
	0x64, 0x65, 0x72, 0x00
};

void ACB::Read(const std::string& inPath)
{
	// read acb
	common_parameter = new ACB_PassParameters();
	common_parameter->path = inPath;

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
	std::cout << "Reading ACB file......\n";
	ReadUTFData(ACB_Buffer, xmlHeader, 1);
	// read memory awb
	if (common_parameter->awb_memory.size()) {
		std::cout << "Reading memory AWB files......\n";
		tinyxml2::XMLElement* xmlAWB = xmlHeader->InsertNewChildElement("AWB");
		xmlAWB->SetAttribute("isStream", "0");
		ReadAWBData(common_parameter->awb_memory, xmlAWB);

		common_parameter->awb_memory.clear();
	}

	// read stream awb
	tempPath = inPath + ".awb";
	if (std::filesystem::exists(tempPath)) {
		std::cout << "Reading stream AWB files......\n";

		std::ifstream awb_file(tempPath, std::ios::binary | std::ios::ate | std::ios::in);
		std::streamsize awb_size = awb_file.tellg();
		awb_file.seekg(0, std::ios::beg);
		std::vector<char> awb_buffer(awb_size);

		ReadFileState = 0;
		if (awb_file.read(awb_buffer.data(), awb_size)) {
			ReadFileState = 1;
		}
		awb_file.close();

		if (ReadFileState) {
			tinyxml2::XMLElement* xmlAWB = xmlHeader->InsertNewChildElement("AWB");
			xmlAWB->SetAttribute("isStream", "1");
			ReadAWBData(awb_buffer, xmlAWB);
		}
		awb_buffer.clear();
	}

	// write file
	std::string outfile = inPath + ".xml";
	xml.SaveFile(outfile.c_str());

	delete common_parameter;
}

void ACB::ReadUTFData(const std::vector<char>& buffer, tinyxml2::XMLElement* xmlHeader, int IsHeader)
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

	if (IsHeader) {
		tinyxml2::XMLElement* xmldata = xmlHeader->InsertNewChildElement("main");
		ReadUTFParameterData(buffer, xmldata, 0, 2);
	}
	else {
		tinyxml2::XMLElement* xmldata = xmlHeader->InsertNewChildElement("static");
		ReadUTFParameterData(buffer, xmldata, 0, 0);

		for (int i = 0; i < v_header.NodeCount; i++) {
			xmldata = xmlHeader->InsertNewChildElement("node");
			xmldata->SetAttribute("index", i);
			ReadUTFParameterData(buffer, xmldata, i, 1);
		}
	}
	// end
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

void ACB::ReadUTFParameterData(const std::vector<char>& in, tinyxml2::XMLElement* xmldata, int index, int type)
{
	UTF_GetParameters inPtr;
	inPtr.type = type;
	for (int i = 0; i < v_parameters.size(); i++) {
		if (v_parameters[i].flag & COLUMN_FLAG_DEFAULT) {
			inPtr.xmlNode = xmldata;
			inPtr.index = i;
			inPtr.isNodePtr = 0;
			inPtr.PtrDataOfs = v_parameters[i].offset;
		}
		else if (v_parameters[i].flag & COLUMN_FLAG_ROW) {
			inPtr.xmlNode = xmldata;
			inPtr.index = i;
			inPtr.isNodePtr = 1;
			inPtr.PtrDataOfs = v_parameters[i].offset + (index * v_header.NodeDataSize);
		}
		else {
			inPtr.xmlNode = 0;
		}

		if (inPtr.xmlNode) {
			ReadUTFParameter_Value(in, inPtr);
		}
	}
}

void ACB::ReadUTFParameter_Value(const std::vector<char>& in, const UTF_GetParameters& inPtr)
{
	int dataType = v_parameters[inPtr.index].type;
	switch (dataType) {
	case COLUMN_TYPE_UINT8:
	case COLUMN_TYPE_SINT8:
	case COLUMN_TYPE_UINT16:
	case COLUMN_TYPE_SINT16:
	case COLUMN_TYPE_UINT32:
	case COLUMN_TYPE_SINT32:
	case COLUMN_TYPE_UINT64:
	case COLUMN_TYPE_SINT64:
		ReadUTFParameter_Int(in, inPtr, dataType);
		break;
	case COLUMN_TYPE_FLOAT:
		ReadUTFParameter_FP32(in, inPtr);
		break;
	case COLUMN_TYPE_STRING:
		ReadUTFParameter_String(in, inPtr);
		break;
	case COLUMN_TYPE_VLDATA:
		ReadUTFParameter_ToData(in, inPtr);
		break;
	default:
		break;
	}
}

void ACB::ReadUTFParameter_Int(const std::vector<char>& in, const UTF_GetParameters& inPtr, int dataType)
{
	int outputValue = 0;
	if (inPtr.type == 2) {
		outputValue = 1;
	}
	else {
		if (inPtr.type == inPtr.isNodePtr) {
			if (inPtr.type == 1) {
				outputValue = -1;
			}
			else {
				outputValue = 1;
			}
		}
		else {
			if (inPtr.type == 1) {
				return;
			}
		}
	}

	int curpos = inPtr.PtrDataOfs;
	tinyxml2::XMLElement* xmlNode = inPtr.xmlNode;
	const UTF_Column_t& data = v_parameters[inPtr.index];

	int wordSize;
	INT64 s64;
	UINT64 u64;
	int isBigEndian = 1;

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

	tinyxml2::XMLElement* xmlptr;
	if (outputValue >= 0) {
		xmlptr = xmlNode->InsertNewChildElement("int");
	}
	else {
		xmlptr = xmlNode->InsertNewChildElement("var");
	}
	xmlptr->SetAttribute("name", data.name);
	if (wordSize > 0) {
		if (outputValue >= 0) {
			xmlptr->SetAttribute("size", wordSize);
			xmlptr->SetAttribute("sign", "0");
		}

		if (outputValue) {
			xmlptr->SetAttribute("value", u64);
		}
	}
	else {
		if (outputValue >= 0) {
			xmlptr->SetAttribute("size", -wordSize);
			xmlptr->SetAttribute("sign", "1");
		}

		if (outputValue) {
			xmlptr->SetAttribute("value", s64);
		}
	}
}

void ACB::ReadUTFParameter_FP32(const std::vector<char>& in, const UTF_GetParameters& inPtr)
{
	int outputValue = 0;
	if (inPtr.type == 2) {
		outputValue = 1;
	}
	else {
		if (inPtr.type == inPtr.isNodePtr) {
			if (inPtr.type == 1) {
				outputValue = -1;
			}
			else {
				outputValue = 1;
			}
		}
		else {
			if (inPtr.type == 1) {
				return;
			}
		}
	}

	int curpos = inPtr.PtrDataOfs;
	tinyxml2::XMLElement* xmlNode = inPtr.xmlNode;
	const UTF_Column_t& data = v_parameters[inPtr.index];

	UINT32 u32 = ReadUINT32(&in[curpos], 1);
	float f32 = *(float*)&u32;

	tinyxml2::XMLElement* xmlptr;
	if (outputValue >= 0) {
		xmlptr = xmlNode->InsertNewChildElement("float");
	}
	else {
		xmlptr = xmlNode->InsertNewChildElement("var");
	}
	xmlptr->SetAttribute("name", data.name);
	if (outputValue) {
		xmlptr->SetAttribute("value", f32);
	}
}

void ACB::ReadUTFParameter_String(const std::vector<char>& in, const UTF_GetParameters& inPtr)
{
	int outputValue = 0;
	if (inPtr.type == 2) {
		outputValue = 1;
	}
	else {
		if (inPtr.type == inPtr.isNodePtr) {
			if (inPtr.type == 1) {
				outputValue = -1;
			}
			else {
				outputValue = 1;
			}
		}
		else {
			if (inPtr.type == 1) {
				return;
			}
		}
	}

	int curpos = inPtr.PtrDataOfs;
	tinyxml2::XMLElement* xmlNode = inPtr.xmlNode;
	const UTF_Column_t& data = v_parameters[inPtr.index];

	tinyxml2::XMLElement* xmlptr;
	if (outputValue >= 0) {
		xmlptr = xmlNode->InsertNewChildElement("string");
	}
	else {
		xmlptr = xmlNode->InsertNewChildElement("var");
	}
	xmlptr->SetAttribute("name", data.name);
	if (outputValue) {
		curpos = ReadUINT32(&in[curpos], 1) + v_header.NameTableOffset;
		std::string str = EscapeControlChars(std::string(&in[curpos]));
		xmlptr->SetAttribute("value", str.c_str());
	}
}

void ACB::ReadUTFParameter_ToData(const std::vector<char>& in, const UTF_GetParameters& inPtr)
{
	int outputValue = 0;
	if (inPtr.type == 2) {
		outputValue = 1;
	}
	else {
		if (inPtr.type == inPtr.isNodePtr) {
			if (inPtr.type == 1) {
				outputValue = -1;
			}
			else {
				outputValue = 1;
			}
		}
		else {
			if (inPtr.type == 1) {
				return;
			}
		}
	}

	int curpos = inPtr.PtrDataOfs;
	tinyxml2::XMLElement* xmlNode = inPtr.xmlNode;
	const UTF_Column_t& data = v_parameters[inPtr.index];

	tinyxml2::XMLElement* xmlptr;
	if (outputValue >= 0) {
		xmlptr = xmlNode->InsertNewChildElement("data");
	}
	else {
		xmlptr = xmlNode->InsertNewChildElement("var");
	}

	xmlptr->SetAttribute("name", data.name);
	if (!outputValue) {
		return;
	}

	int dataPos = ReadUINT32(&in[curpos], 1) + v_header.DataOffset;
	int dataSize = ReadUINT32(&in[curpos+4], 1);

	if (!dataSize) {
		xmlptr->SetAttribute("value", "NUL");
		return;
	}

	if (dataPos >= v_header.BlockSize) {
		xmlptr->SetAttribute("debug", curpos);
		xmlptr->SetAttribute("value", "NUL");
		return;
	}

	if (!strcmp(data.name, "AwbFile")) {
		xmlptr->SetAttribute("value", "MEM");
		common_parameter->awb_memory.assign(in.begin() + dataPos, in.begin() + dataPos + dataSize);
		return;
	}else if (!strcmp(data.name, "StreamAwbAfs2Header")) {
		xmlptr->SetAttribute("value", "MAP");
		return;
	}

	if (*(UINT32*)&in[dataPos] == 1179931968) {
		xmlptr->SetAttribute("value", "UTF");

		std::vector<char> newbuf(in.begin() + dataPos, in.begin() + dataPos + dataSize);

		std::unique_ptr< ACB > pACB = std::make_unique< ACB >();
		pACB->common_parameter = common_parameter;
		pACB->ReadUTFData(newbuf, xmlptr, 0);
		pACB.reset();
	}
	else {
		xmlptr->SetAttribute("value", "RAW");
		std::string rawData = RawDataToHexString(&in[dataPos], dataSize);
		xmlptr->SetText(rawData.c_str());
	}
}

void ACB::ReadAWBData(const std::vector<char>& in, tinyxml2::XMLElement* xmldata)
{
	std::unique_ptr< AWB > pAWB = std::make_unique< AWB >();

	// get header
	memcpy(&pAWB->v_header, &in[0], 0x10);
	// AFS2
	if (pAWB->v_header.header != 844318273) {
		std::cout << "This is not an AWB file!\n";
		pAWB.reset();
		return;
	}
	// get data offset
	pAWB->ReadDataOffset(in);

	// check folder
	std::string filePath = common_parameter->path;
	int FolderExists = DirectoryExists(filePath.c_str());
	if (!FolderExists) {
		CreateDirectoryA(filePath.c_str(), NULL);
	}

	// output file
	for (int i = 0; i < pAWB->v_DataFile.size(); i++) {
		std::string fileName = std::format("{:08x}_{:08X}", in.size(), pAWB->v_DataFile[i].cueID);

		tinyxml2::XMLElement* xmlNode = xmldata->InsertNewChildElement("cue");
		xmlNode->SetAttribute("ID", pAWB->v_DataFile[i].cueID);
		xmlNode->SetAttribute("File", fileName.c_str());

		int selfOfs = pAWB->v_DataFile[i].self;
		int nextOfs = pAWB->v_DataFile[i].next;
		std::vector<char> v_data(in.begin() + selfOfs, in.begin() + nextOfs);
		std::ofstream newFile(filePath + "\\" + fileName + ".hca", std::ios::binary | std::ios::out | std::ios::ate);
		newFile.write(v_data.data(), v_data.size());
		newFile.close();
	}

	// end
	pAWB.reset();
}
