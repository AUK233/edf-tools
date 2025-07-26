#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>
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

	// read waveform name
	// now, don't use it
	//UTF_WaveformName_t* pWaveformName = new UTF_WaveformName_t();
	//ReadAWBWaveformName(xmlHeader, pWaveformName);
	//delete pWaveformName;

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
	std::cout << "Complete!\n";
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

	// check whether the first name is used
	std::string firstName = ReadUTF1stString(buffer, v_header.NameTableOffset, v_header.DataOffset);
	name_map[firstName] = 0;

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

	// if first name is not used, set to block name
	if (name_map[firstName] == 0) {
		xmlHeader->SetAttribute("UTFName", firstName.c_str());
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

std::string ACB::ReadUTF1stString(const std::vector<char>& in, int inStart, int inEnd)
{
	// get string list
	int offset = inStart;
	int size = 0;
	for (int i = inStart; i < inEnd; i++) {
		size++;
		if (in[i] == 0) {
			break;
		}
	}
	return EscapeControlChars(std::string(&in[offset], size));
}

void ACB::CheckStringIsUsed(const std::string& in)
{
	if (name_map.find(in) != name_map.end()) {
		name_map[in]++;
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
	CheckStringIsUsed(v_parameters[inPtr.index].name);

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
	} else if (!strcmp(data.name, "StreamAwbAfs2Header")) {
		xmlptr->SetAttribute("value", "MAP");
		return;
	}
	else if (!strcmp(data.name, "Command")) {
		xmlptr->SetAttribute("value", "CMD");
		std::vector<char> cmdBuffer(in.begin() + dataPos, in.begin() + dataPos + dataSize);
		ReadUTFData_CommandCode(cmdBuffer, xmlptr);
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

void ACB::ReadUTFData_CommandCode(const std::vector<char>& in, tinyxml2::XMLElement* xmldata)
{
	int inSize = in.size();
	int curpos = 0;
	while (curpos < inSize) {
		// get command
		UTF_Command_t cmdCode;
		cmdCode.cmdType = ReadUINT16(&in[curpos], 1);
		cmdCode.valueSize = in[curpos + 2];
		curpos += 3;

		tinyxml2::XMLElement* xmlptr;
		switch (cmdCode.cmdType) {
		case 0x57: {
			if (cmdCode.valueSize != 2) {
				goto defaultCMD;
			}

			xmlptr = xmldata->InsertNewChildElement("cmd");
			xmlptr->SetAttribute("code", cmdCode.cmdType);

			// get data
			INT16 volume = ReadUINT16(&in[curpos], 1);
			xmlptr->SetAttribute("volume", volume);

			xmlptr->SetAttribute("note", "adjust volume, but it is an integer that will divide by 100 when game reads it");
			break;
		}
		case 0x7D0: {
			if (cmdCode.valueSize != 4) {
				goto defaultCMD;
			}

			xmlptr = xmldata->InsertNewChildElement("cmd");
			xmlptr->SetAttribute("code", cmdCode.cmdType);

			// get data
			INT16 wavetype = ReadUINT16(&in[curpos], 1);
			INT16 waveId = ReadUINT16(&in[curpos + 2], 1);
			xmlptr->SetAttribute("wavetype", wavetype);
			xmlptr->SetAttribute("waveId", waveId);

			xmlptr->SetAttribute("note", "get wave from waveform id");
			break;
		}
		default: {
			defaultCMD:
			xmlptr = xmldata->InsertNewChildElement("opcode");
			xmlptr->SetAttribute("type", cmdCode.cmdType);
			//xmlptr->SetAttribute("size", cmdCode.valueSize);
			if (cmdCode.valueSize) {
				std::string rawData = RawDataToHexString(&in[curpos], cmdCode.valueSize);
				xmlptr->SetText(rawData.c_str());
			}
			break;
		}
		}
		//
		curpos += cmdCode.valueSize;
	}
}

void ACB::ReadAWBWaveformName(tinyxml2::XMLElement* xmlHeader, UTF_WaveformName_t* pName)
{
	tinyxml2::XMLElement* xml_main = xmlHeader->FirstChildElement("main");

	// get sequence table
	tinyxml2::XMLElement* xml_data = ReadAWBWaveformName_GetNode(xml_main, "SequenceTable");
	if (xml_data == 0) {
		return;
	}
	std::vector<UTF_Sequence_t> v_Sequence;
	ReadAWBWaveformName_GetSequence(xml_data, v_Sequence);

	// get cue name
	xml_data = ReadAWBWaveformName_GetNode(xml_main, "CueNameTable");
	if (xml_data == 0) {
		return;
	}
	ReadAWBWaveformName_GetCueName(xml_data, v_Sequence);

	// in TrackEventTable, 07d0040002 followed by a BE int16 is synth index.
}

tinyxml2::XMLElement* ACB::ReadAWBWaveformName_GetNode(tinyxml2::XMLElement* xmldata, const char* name)
{
	for (tinyxml2::XMLElement* entry = xmldata->FirstChildElement(); entry != 0; entry = entry->NextSiblingElement()) {
		if (!strcmp(entry->Attribute("name"), name)) {
			return entry;
		}
	}
	return nullptr;
}

void ACB::ReadAWBWaveformName_GetSequence(tinyxml2::XMLElement* xmldata, std::vector<UTF_Sequence_t>& v_Sequence)
{
	tinyxml2::XMLElement* xml_NumTracks = ReadAWBWaveformName_GetNode(xmldata->FirstChildElement("static"), "NumTracks");
	int numTracks = xml_NumTracks->IntAttribute("value", 0);

	tinyxml2::XMLElement* entry, *xml_track;
	for (entry = xmldata->FirstChildElement("node"); entry != 0; entry = entry->NextSiblingElement("node")) {
		UTF_Sequence_t out;
		out.cue_name = "";
		if (numTracks) {
			xml_track = ReadAWBWaveformName_GetNode(entry, "TrackIndex");

			std::vector<char> buffer;
			if (xml_track) {
				buffer = HexStringToRawData(xml_track->GetText());
			}
			//
			if (buffer.size()) {
				for(int i = 0; i < buffer.size(); i+=4) {
					int trackID = ReadUINT16(&buffer[i], 1);
					out.v_waveform.push_back(trackID);
				}
			}
		}
		v_Sequence.push_back(out);
	}
}

void ACB::ReadAWBWaveformName_GetCueName(tinyxml2::XMLElement* xmldata, std::vector<UTF_Sequence_t>& v_Sequence)
{
	tinyxml2::XMLElement* entry, * xml_name, * xml_index;
	for (entry = xmldata->FirstChildElement("node"); entry != 0; entry = entry->NextSiblingElement("node")) {
		xml_index = ReadAWBWaveformName_GetNode(entry, "CueIndex");
		if (xml_index == 0) {
			continue;
		}

		xml_name = ReadAWBWaveformName_GetNode(entry, "CueName");
		if (xml_name) {
			int index = xml_index->IntAttribute("value", 0);
			v_Sequence[index].cue_name = xml_name->Attribute("value");
		}
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
		std::string fileName = std::format("{:08x}_{:08X}.hca", in.size(), pAWB->v_DataFile[i].cueID);

		tinyxml2::XMLElement* xmlNode = xmldata->InsertNewChildElement("cue");
		xmlNode->SetAttribute("ID", pAWB->v_DataFile[i].cueID);
		xmlNode->SetAttribute("File", fileName.c_str());

		int selfOfs = pAWB->v_DataFile[i].self;
		int nextOfs = pAWB->v_DataFile[i].next;
		std::vector<char> v_data(in.begin() + selfOfs, in.begin() + nextOfs);
		std::ofstream newFile(filePath + "\\" + fileName, std::ios::binary | std::ios::out | std::ios::ate);
		newFile.write(v_data.data(), v_data.size());
		newFile.close();
	}

	// end
	pAWB.reset();
}

// ============================================================================================================================

void ACB::Write(const std::string& inPath)
{
	// write
	common_parameter = new ACB_PassParameters();
	common_parameter->path = inPath;

	// read xml
	std::string xmlPath = inPath + ".xml";
	tinyxml2::XMLDocument doc;
	doc.LoadFile(xmlPath.c_str());
	tinyxml2::XMLElement* header = doc.FirstChildElement("ACB");

	// read awb data
	tinyxml2::XMLElement* awbData = header->FirstChildElement("AWB");
	if (awbData) {
		WriteAWBDataCheck(awbData);

		// this needs to be read twice.
		awbData = awbData->NextSiblingElement("AWB");
		if (awbData) {
			WriteAWBDataCheck(awbData);
		}
	}

	// read acb data
	std::cout << "Reading ACB data......\n";
	//tinyxml2::XMLElement* mainData = header->FirstChildElement("main");
	std::vector<char> bytes = WriteUTFData(header, 1);

	// write to file
	std::ofstream newFile(common_parameter->path + ".acb", std::ios::binary | std::ios::out | std::ios::ate);
	newFile.write(bytes.data(), bytes.size());
	newFile.close();

	delete common_parameter;
	std::cout << "Done!\n";
}

void ACB::WriteAWBDataCheck(tinyxml2::XMLElement* xmldata)
{
	int isStream = xmldata->IntAttribute("isStream", 1);
	if (!isStream) {
		std::cout << "Reading memory AWB file......\n";
		common_parameter->awb_memory = WriteAWBDataGet(xmldata, 0);
		return;
	}
	else {
		std::cout << "Reading stream AWB file......\n";
		int headerSize = 0;
		std::vector<char> stream = WriteAWBDataGet(xmldata, &headerSize);
		// write to file
		std::ofstream newFile(common_parameter->path + ".awb", std::ios::binary | std::ios::out | std::ios::ate);
		newFile.write(stream.data(), stream.size());
		newFile.close();

		// set stream mapping
		std::vector<char> mapping(std::begin(RAW_StreamAwbAfs2Header), std::end(RAW_StreamAwbAfs2Header));
		WriteINT32BE(&mapping[0x29], headerSize);
		mapping.insert(mapping.end(), stream.begin(), stream.begin() + headerSize);
		stream.clear();

		int streamSize = mapping.size();
		int align = streamSize % 16;
		if (align) {
			streamSize += 16 - align;
		}
		WriteINT32BE(&mapping[0x4], streamSize-8);

		// check 32-byte alignment
		streamSize = mapping.size();
		align = streamSize % 32;
		if (align) {
			for (int i = align; i < 32; i++) {
				mapping.push_back(0);
			}
		}

		common_parameter->awb_stream = mapping;
		return;
	}
}

std::vector<char> ACB::WriteAWBDataGet(tinyxml2::XMLElement* xmldata, int* headerSize)
{
	struct WaveData_t {
		std::vector<char> data;
		int CueID;
		int offset_data;
	};
	std::vector<WaveData_t> v_wave;

	// read cue file
	std::cout << "  Getting files:\n0";
	std::string filePath = common_parameter->path + "\\";
	int i_DataSize = 0;
	for (tinyxml2::XMLElement* entry = xmldata->FirstChildElement("cue"); entry != 0; entry = entry->NextSiblingElement("cue"))
	{
		WaveData_t out;
		out.CueID = entry->IntAttribute("ID", v_wave.size());
		out.offset_data = i_DataSize;
		// read file
		std::string path = filePath + entry->Attribute("File");
		std::ifstream file(path, std::ios::binary | std::ios::ate | std::ios::in);
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);
		out.data.resize(size);
		file.read(out.data.data(), size);
		file.close();

		// set value
		// 32-byte alignment in here
		int align_buffer = out.data.size() % 32;
		if (align_buffer) {
			for (int i = align_buffer; i < 32; i++) {
				out.data.push_back(0);
			}
		}
		i_DataSize += out.data.size();

		v_wave.push_back(out);

		std::cout << "\r   " + std::to_string(v_wave.size());
	}
	int FileCount = v_wave.size();

	// set header
	std::cout << "\n  Writing buffer......\n";
	struct awb_header_t {
		UINT32 header = 844318273;
		BYTE pad4 = 1;
		BYTE dataOfs_size = 4;
		// here we use the 2-byte, this way it can be read by EAT
		UINT16 cueID_size = 2;
		INT32 sound_count = -2;
		UINT32 align_size = 32;
	} awb_header;
	awb_header.sound_count = FileCount;

	int CueIDSize = FileCount * 2;
	int DataOfsSize = FileCount * 4;

	// points to EOF requires an extra data
	int HeaderSize = 0x10 + CueIDSize + DataOfsSize + 4;
	int align = HeaderSize % 32;
	if (align) {
		HeaderSize += 32 - align;
	}
	// return header size
	if (headerSize) {
		*headerSize = HeaderSize+4;
	}

	// write buffer
	std::vector<char> bytes(HeaderSize, 0);
	memcpy(&bytes[0], &awb_header, 0x10);

	// write waveform
	int cur_CueID = 0x10;
	int cur_DataOfs = 0x10 + CueIDSize;
	for (int i = 0; i < v_wave.size(); i++) {
		// write 2-byte
		WriteINT16LE(&bytes[cur_CueID], v_wave[i].CueID);

		int dataOfs = HeaderSize + v_wave[i].offset_data;
		WriteINT32LE(&bytes[cur_DataOfs], dataOfs);

		cur_CueID += 2;
		cur_DataOfs += 4;

		bytes.insert(bytes.end(), v_wave[i].data.begin(), v_wave[i].data.end());
		v_wave[i].data.clear();
	}
	// write eof
	WriteINT32LE(&bytes[cur_DataOfs], bytes.size());

	std::cout << " Complete!\n";
	return bytes;
}

std::vector<char> ACB::WriteUTFData(tinyxml2::XMLElement* xmldata, int isHeader)
{
	// set header
	UTF_Header_t* acb_header = 0;
	WriteUTFData_InitHeader(&acb_header);
	if (!acb_header) {
		return std::vector<char>();
	}
	// set block
	UTF_Data_t* p_data = new UTF_Data_t();
	p_data->var_size = 0;
	p_data->parameter_count = 0;
	p_data->node_count = 0;

	// write block name
	auto UTFName = xmldata->Attribute("UTFName");
	if (UTFName) {
		WriteUTFData_WriteString(p_data->v_string, p_data->string_map, UTFName);
	}

	// get data
	if (isHeader) {
		//WriteUTFData_WriteString(p_data->v_string, p_data->string_map, "Header");

		tinyxml2::XMLElement* xml_main = xmldata->FirstChildElement("main");
		WriteUTFData_InitParameterName(xml_main, p_data);
		WriteUTFNode(xml_main, p_data, 2);
		p_data->node_count = 1;
	}
	else {
		// first, get parameter list
		tinyxml2::XMLElement* xml_static = xmldata->FirstChildElement("static");
		if (!xml_static) {
			return std::vector<char>();
		}

		WriteUTFData_InitParameterName(xml_static, p_data);
		WriteUTFNode(xml_static, p_data, 0);

		// next, get variable node list
		for (tinyxml2::XMLElement* xml_node = xmldata->FirstChildElement("node"); xml_node != 0; xml_node = xml_node->NextSiblingElement("node")) {
			WriteUTFNode(xml_node, p_data, 1);
			p_data->node_count++;
		}
		// end
	}

	//
	std::vector<char> bytes(0x20, 0);
	// write header
	memcpy(&bytes[0], acb_header, 0x20);
	_aligned_free(acb_header);

	// write parameter
	bytes.insert(bytes.end(), p_data->v_parameter.begin(), p_data->v_parameter.end());
	p_data->v_parameter.clear();

	// write node data
	UINT16 nodeOfs = bytes.size() - 8;
	WriteINT16BE(&bytes[10], nodeOfs);
	bytes.insert(bytes.end(), p_data->v_node.begin(), p_data->v_node.end());
	p_data->v_node.clear();

	// write string
	UINT32 stringOfs = bytes.size() - 8;
	WriteINT32BE(&bytes[12], stringOfs);
	bytes.insert(bytes.end(), p_data->v_string.begin(), p_data->v_string.end());
	p_data->v_string.clear();

	// check 32-byte alignment
	int align = bytes.size() % 32;
	if (align) {
		for (int i = align; i < 32; i++) {
			bytes.push_back(0);
		}
	}

	// write data
	UINT32 dataOfs = bytes.size() - 8;
	WriteINT32BE(&bytes[16], dataOfs);
	bytes.insert(bytes.end(), p_data->v_data.begin(), p_data->v_data.end());
	p_data->v_data.clear();
	align = bytes.size() % 32;
	if (align) {
		for (int i = align; i < 32; i++) {
			bytes.push_back(0);
		}
	}

	// get size
	UINT32 size = bytes.size() - 8;
	WriteINT32BE(&bytes[4], size);

	// set other
	WriteINT16BE(&bytes[24], p_data->parameter_count);
	WriteINT16BE(&bytes[26], p_data->var_size);
	WriteINT32BE(&bytes[28], p_data->node_count);

	delete p_data;
	return bytes;
}

void ACB::WriteUTFData_InitHeader(UTF_Header_t** p)  
{  
	// release old pointer.  
	if (*p) {
		_aligned_free(*p);
	}

	*p = (UTF_Header_t*)_aligned_malloc(sizeof(UTF_Header_t), 16);
	if (!*p) {
		return;
	}

	(*p)->U32_header = 1179931968;
	(*p)->Version = 256;
	(*p)->NameOffset = 0;
}

int ACB::WriteUTFData_WriteString(std::vector<char>& buffer, StringToIntMap& map, const std::string& str)
{
	// empty string is necessary
	std::string sv;
	if (str.empty()) {
		sv = str;
	}
	else {
		sv = UnescapeControlChars(str);
	}
	// check map
	auto it = map.find(sv);
	if (it != map.end()) {
		return it->second;
	}

	// write string
	int offset = buffer.size();
	map[sv] = offset;

	buffer.insert(buffer.end(), sv.begin(), sv.end());
	buffer.push_back(0);

	return offset;
}

void ACB::WriteUTFData_InitParameterName(tinyxml2::XMLElement* xmldata, UTF_Data_t* p_data)
{
	for (tinyxml2::XMLElement* entry = xmldata->FirstChildElement(); entry != 0; entry = entry->NextSiblingElement())
	{
		std::string name = entry->Attribute("name");
		WriteUTFData_WriteString(p_data->v_string, p_data->string_map, name);
	}
}

void ACB::WriteUTFNode(tinyxml2::XMLElement* xmldata, UTF_Data_t* p_data, int type)
{
	// get node
	tinyxml2::XMLElement* entry;
	if (type == 1) {
		std::vector<char> var_bytes(p_data->var_size, 0);
		for (entry = xmldata->FirstChildElement("var"); entry != 0; entry = entry->NextSiblingElement("var"))
		{
			WriteUTFNodeData(entry, p_data, var_bytes);
		}
		p_data->v_node.insert(p_data->v_node.end(), var_bytes.begin(), var_bytes.end());
	}
	else {
		for (entry = xmldata->FirstChildElement(); entry != 0; entry = entry->NextSiblingElement())
		{
			WriteUTFParameter(entry, p_data, type);
			p_data->parameter_count++;
		}
	}
}

void ACB::WriteUTFParameter(tinyxml2::XMLElement* xmldata, UTF_Data_t* p_data, int isHeader)
{
	UTF_PTRData_t ptr_data;
	// get node name
	std::string nodeName = xmldata->Name();
	if (nodeName == "int")
	{
		int sign = xmldata->IntAttribute("sign", 0);
		int size = xmldata->IntAttribute("size", 0);
		if (!sign) {
			switch (size)
			{
			case 1:
				ptr_data.data_type = COLUMN_TYPE_UINT8;
				ptr_data.data_size = 1;
				break;
			case 2:
				ptr_data.data_type = COLUMN_TYPE_UINT16;
				ptr_data.data_size = 2;
				break;
			case 4:
				ptr_data.data_type = COLUMN_TYPE_UINT32;
				ptr_data.data_size = 4;
				break;
			case 8:
				ptr_data.data_type = COLUMN_TYPE_UINT64;
				ptr_data.data_size = 8;
				break;
			default:
				return;
			}
		}
		else {
			switch (size)
			{
			case 1:
				ptr_data.data_type = COLUMN_TYPE_SINT8;
				ptr_data.data_size = 1;
				break;
			case 2:
				ptr_data.data_type = COLUMN_TYPE_SINT16;
				ptr_data.data_size = 2;
				break;
			case 4:
				ptr_data.data_type = COLUMN_TYPE_SINT32;
				ptr_data.data_size = 4;
				break;
			case 8:
				ptr_data.data_type = COLUMN_TYPE_SINT64;
				ptr_data.data_size = 8;
				break;
			default:
				return;
			}
		}
		// end
	}
	else if (nodeName == "float")
	{
		ptr_data.data_type = COLUMN_TYPE_FLOAT;
		ptr_data.data_size = 4;
	}
	else if (nodeName == "string")
	{
		ptr_data.data_type = COLUMN_TYPE_STRING;
		ptr_data.data_size = 4;
	}
	else if (nodeName == "data")
	{
		ptr_data.data_type = COLUMN_TYPE_VLDATA;
		ptr_data.data_size = 8;
	}
	else {
		return;
	}

	UINT8 type = ptr_data.data_type | COLUMN_FLAG_NAME;
	char name_offset[4];
	std::string name = xmldata->Attribute("name");
	int offset = WriteUTFData_WriteString(p_data->v_string, p_data->string_map, name);
	WriteINT32BE(&name_offset[0], offset);

	// check if it is a variable
	auto hasValue = xmldata->FindAttribute("value");
	if (isHeader) {
		type |= COLUMN_FLAG_ROW;
	}
	else {
		if(hasValue) {
			type |= COLUMN_FLAG_DEFAULT;
		}
		else {
			type |= COLUMN_FLAG_ROW;
		}
	}

	// write value
	int type_pos = p_data->v_parameter.size();
	p_data->v_parameter.push_back(type);
	p_data->v_parameter.insert(p_data->v_parameter.end(), name_offset, name_offset + 4);

	if (hasValue) {
		WriteUTFParameterData(xmldata, p_data, &ptr_data);

		if (isHeader) {
			p_data->v_node.insert(p_data->v_node.end(), ptr_data.buffer, ptr_data.buffer + ptr_data.data_size);
			p_data->var_size += ptr_data.data_size;
		}
		else {
			p_data->v_parameter.insert(p_data->v_parameter.end(), ptr_data.buffer, ptr_data.buffer + ptr_data.data_size);
		}
	}
	else {
		// set information for this parameter
		UTF_Data_ptr_t out_ptr;
		out_ptr.data_type = ptr_data.data_type;
		out_ptr.data_size = ptr_data.data_size;
		out_ptr.data_offset = p_data->var_size;

		int table_pos = p_data->v_ptrTable.size();
		p_data->ptrTable_map[name] = table_pos;
		p_data->v_ptrTable.push_back(out_ptr);
		p_data->var_size += ptr_data.data_size;
	}
}

void ACB::WriteUTFParameterData(tinyxml2::XMLElement* xmldata, UTF_Data_t* p_data, UTF_PTRData_t* p)
{
	switch (p->data_type)
	{
	case COLUMN_TYPE_UINT8: {
		UINT8 u8 = xmldata->IntAttribute("value", 0);
		p->buffer[0] = u8;
		break;
	}
	case COLUMN_TYPE_SINT8: {
		INT8 s8 = xmldata->IntAttribute("value", 0);
		p->buffer[0] = s8;
		break;
	}
	case COLUMN_TYPE_UINT16: {
		UINT16 u16 = xmldata->IntAttribute("value", 0);
		WriteINT16BE(&p->buffer[0], u16);
		break;
	}
	case COLUMN_TYPE_SINT16: {
		INT16 s16 = xmldata->IntAttribute("value", 0);
		WriteINT16BE(&p->buffer[0], s16);
		break;
	}
	case COLUMN_TYPE_UINT32: {
		UINT32 u32 = xmldata->UnsignedAttribute("value", 0);
		WriteINT32BE(&p->buffer[0], u32);
		break;
	}
	case COLUMN_TYPE_SINT32: {
		INT32 s32 = xmldata->IntAttribute("value", 0);
		WriteINT32BE(&p->buffer[0], s32);
		break;
	}
	case COLUMN_TYPE_UINT64: {
		UINT64 u64 = xmldata->Unsigned64Attribute("value", 0);
		WriteINT64BE(&p->buffer[0], u64);
		break;
	}
	case COLUMN_TYPE_SINT64: {
		INT64 s64 = xmldata->Int64Attribute("value", 0);
		WriteINT64BE(&p->buffer[0], s64);
		break;
	}
	case COLUMN_TYPE_FLOAT: {
		float fp32 = xmldata->FloatAttribute("value", 0);
		WriteINT32BE(&p->buffer[0], *(INT32*)&fp32);
		break;
	}
	case COLUMN_TYPE_STRING: {
		std::string str = xmldata->Attribute("value");
		int offset = WriteUTFData_WriteString(p_data->v_string, p_data->string_map, str);
		WriteINT32BE(&p->buffer[0], offset);
		break;
	}
	case COLUMN_TYPE_VLDATA: {
		WriteUTFNodePTRData(xmldata, p_data, &p->buffer[0]);
		break;
	}
	default:
		break;
	}
}

void ACB::WriteUTFNodeData(tinyxml2::XMLElement* xmldata, UTF_Data_t* p_data, std::vector<char>& buffer)
{
	// get node name
	std::string name = xmldata->Attribute("name");

	auto it = p_data->ptrTable_map.find(name);
	if (it == p_data->ptrTable_map.end()) {
		return;
	}
	int index = it->second;

	int data_type = p_data->v_ptrTable[index].data_type;
	int data_offset = p_data->v_ptrTable[index].data_offset;

	switch (data_type)
	{
	case COLUMN_TYPE_UINT8: {
		UINT8 u8 = xmldata->IntAttribute("value", 0);
		buffer[data_offset] = u8;
		break;
	}
	case COLUMN_TYPE_SINT8: {
		INT8 s8 = xmldata->IntAttribute("value", 0);
		buffer[data_offset] = s8;
		break;
	}
	case COLUMN_TYPE_UINT16: {
		UINT16 u16 = xmldata->IntAttribute("value", 0);
		WriteINT16BE(&buffer[data_offset], u16);
		break;
	}
	case COLUMN_TYPE_SINT16: {
		INT16 s16 = xmldata->IntAttribute("value", 0);
		WriteINT16BE(&buffer[data_offset], s16);
		break;
	}
	case COLUMN_TYPE_UINT32: {
		UINT32 u32 = xmldata->UnsignedAttribute("value", 0);
		WriteINT32BE(&buffer[data_offset], u32);
		break;
	}
	case COLUMN_TYPE_SINT32: {
		INT32 s32 = xmldata->IntAttribute("value", 0);
		WriteINT32BE(&buffer[data_offset], s32);
		break;
	}
	case COLUMN_TYPE_UINT64: {
		UINT64 u64 = xmldata->Unsigned64Attribute("value", 0);
		WriteINT64BE(&buffer[data_offset], u64);
		break;
	}
	case COLUMN_TYPE_SINT64: {
		INT64 s64 = xmldata->Int64Attribute("value", 0);
		WriteINT64BE(&buffer[data_offset], s64);
		break;
	}
	case COLUMN_TYPE_FLOAT: {
		float fp32 = xmldata->FloatAttribute("value", 0);
		WriteINT32BE(&buffer[data_offset], *(INT32*)&fp32);
		break;
	}
	case COLUMN_TYPE_STRING: {
		std::string str = xmldata->Attribute("value");
		int offset = WriteUTFData_WriteString(p_data->v_string, p_data->string_map, str);
		WriteINT32BE(&buffer[data_offset], offset);
		break;
	}
	case COLUMN_TYPE_VLDATA: {
		WriteUTFNodePTRData(xmldata, p_data, &buffer[data_offset]);
		break;
	}
	default:
		break;
	}
}

void ACB::WriteUTFNodePTRData(tinyxml2::XMLElement* xmldata, UTF_Data_t* p_data, char* buffer)
{
	std::string data_type = xmldata->Attribute("value");
	if (data_type == "UTF") {
		std::vector<char> bytes = WriteUTFData(xmldata, 0);
		int dataSize = bytes.size();
		if (dataSize == 0) {
			goto SetNullValue;
		}

		int dataPos = p_data->v_data.size();
		WriteINT32BE(&buffer[0], dataPos);
		WriteINT32BE(&buffer[4], dataSize);
		p_data->v_data.insert(p_data->v_data.end(), bytes.begin(), bytes.end());
		return;
	}
	else if (data_type == "CMD") {
		int dataSize = 0;
		std::vector<char> bytes = WriteUTF_CommandData(xmldata, &dataSize);
		if (dataSize == 0) {
			goto SetNullValue;
		}

		int dataPos = p_data->v_data.size();
		WriteINT32BE(&buffer[0], dataPos);
		WriteINT32BE(&buffer[4], dataSize);
		p_data->v_data.insert(p_data->v_data.end(), bytes.begin(), bytes.end());
		return;
	}
	else if (data_type == "RAW") {
		std::string argsStrn = xmldata->GetText();
		if (argsStrn.length() % 2 > 0)
		{
			argsStrn = ("0" + argsStrn);
		}
		int dataSize = 0;
		int dataPos = p_data->v_data.size();
		//Convert to hex.
		for (unsigned int i = 0; i < argsStrn.length(); i += 2)
		{
			std::string byteString = argsStrn.substr(i, 2);
			char byte = (char)std::stol(byteString.c_str(), NULL, 16);
			p_data->v_data.push_back(byte);
			dataSize++;
		}

		if (dataSize == 0) {
			goto SetNullValue;
		}
		WriteINT32BE(&buffer[0], dataPos);
		WriteINT32BE(&buffer[4], dataSize);

		if (dataSize > 15) {
			// check 32-byte alignment
			int align = dataSize % 32;
			if (align) {
				for (int i = align; i < 32; i++) {
					p_data->v_data.push_back(0);
				}
			}
		}
		return;
	}
	else if (data_type == "MEM") {
		int dataSize = common_parameter->awb_memory.size();
		if (dataSize == 0) {
			goto SetNullValue;
		}

		int dataPos = p_data->v_data.size();
		WriteINT32BE(&buffer[0], dataPos);
		WriteINT32BE(&buffer[4], dataSize);
		p_data->v_data.insert(p_data->v_data.end(), common_parameter->awb_memory.begin(), common_parameter->awb_memory.end());
		return;
	}
	else if (data_type == "MAP") {
		int dataSize = common_parameter->awb_stream.size();
		if (dataSize == 0) {
			goto SetNullValue;
		}

		int dataPos = p_data->v_data.size();
		WriteINT32BE(&buffer[0], dataPos);
		WriteINT32BE(&buffer[4], dataSize);
		p_data->v_data.insert(p_data->v_data.end(), common_parameter->awb_stream.begin(), common_parameter->awb_stream.end());
		return;
	}
	else {
		SetNullValue:
		*(INT64*)buffer = 0;
		return;
	}
	// end
}

std::vector<char> ACB::WriteUTF_CommandData(tinyxml2::XMLElement* xmldata, int* pOutSize)
{
	std::vector<char> out;
	
	char maxBuffer[32];
	alignas(4) UTF_Command_t cmdCode;
	char codeBuffer[4];
	int noAlign = 0;

	tinyxml2::XMLElement* entry;
	for (entry = xmldata->FirstChildElement(); entry != 0; entry = entry->NextSiblingElement()) {
		int buffSize = 0;
		std::string nodeType = entry->Name();
		if (nodeType == "cmd") {
			cmdCode.cmdType = entry->IntAttribute("code", 0);

			switch (cmdCode.cmdType)
			{
			case 0x57: {
				INT16 volume = entry->IntAttribute("volume", 0);
				WriteINT16BE(&maxBuffer[0], volume);

				buffSize = 2;
				cmdCode.valueSize = buffSize;
				break;
			}
			case 0x7D0: {
				INT16 wavetype = entry->IntAttribute("wavetype", 0);
				INT16 waveId = entry->IntAttribute("waveId", 0);
				WriteINT16BE(&maxBuffer[0], wavetype);
				WriteINT16BE(&maxBuffer[2], waveId);

				buffSize = 4;
				cmdCode.valueSize = buffSize;
				break;
			}
			default: goto toOPcode;
			}
		}
		else if (nodeType == "opcode")
		{
			cmdCode.cmdType = entry->IntAttribute("type", 0);

			toOPcode:
			auto str = entry->GetText();
			if (str) {
				std::string argsStrn = str;
				if (argsStrn.length() % 2 > 0)
				{
					argsStrn = ("0" + argsStrn);
				}

				//Convert to hex.
				for (unsigned int i = 0; i < argsStrn.length(); i += 2)
				{
					if (buffSize >= 32) {
						std::cout << "Warning: Command data is too large, it will be truncated.\n";
						break;
					}

					std::string byteString = argsStrn.substr(i, 2);
					char byte = (char)std::stol(byteString.c_str(), NULL, 16);
					maxBuffer[buffSize] = byte;
					buffSize++;
				}
			}

			cmdCode.valueSize = buffSize;
		}
		else {
			continue;
		}

		WriteINT16BE(&codeBuffer[0], cmdCode.cmdType);
		codeBuffer[2] = cmdCode.valueSize;

		out.insert(out.end(), codeBuffer, codeBuffer + 3);
		if (buffSize) {
			out.insert(out.end(), maxBuffer, maxBuffer + buffSize);
		}

		if( !cmdCode.cmdType && !cmdCode.valueSize) {
			noAlign = 1;
		}
		else {
			noAlign = 0;
		}
	}

	*pOutSize = out.size();
	if (!noAlign) {
		int align = out.size() % 32;
		if (align) {
			ZeroMemory(maxBuffer, 32);
			out.insert(out.end(), maxBuffer, maxBuffer + (32 - align));
		}
	}

	return out;
}
