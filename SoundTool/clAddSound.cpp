#include <string>
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>

#include "clAddSound.h"
#include "clUtil.h"

void AddSound2ACB::AddSound(const std::string& ACBpath, const std::string& FolderPath, const int IsInsert)
{
	// Set insert mode
	InsertMode = IsInsert;

	std::cout << "Getting add files from folder......\n";
	if(!GetAddedFiles(FolderPath)) {
		std::cout << "Error: No files to add.\n";
		system("pause");
		return;
	}

	tinyxml2::XMLDocument doc;
	doc.LoadFile(ACBpath.c_str());
	tinyxml2::XMLElement* header = doc.FirstChildElement("ACB");

	std::cout << "Reading ACB data......\n";
	if (GetAWBStreamNode(header)) {
		std::cout << "Error: Missing stream AWB node.\n";
		system("pause");
		return;
	}

	tinyxml2::XMLElement* mainData = header->FirstChildElement("main");
	NodeErrorCode err = GetUTFNode(mainData);
	if (err != NodeErrorCode::NoError) {
		std::cout << "Error: Missing required node.\n";
		system("pause");
		return;
	}

	// processing data in insert mode
	if (InsertMode) {
		UpdateDataInInsertMode();
	}

	std::cout << "Adding sound data to ACB......\n";
	AddSoundToCueNameTable();
	AddSoundToCueTable();
	AddSoundToWaveformTable();
	AddSoundToSynthTable();
	AddSoundToTrackTable();
	AddSoundToTrackEventTable();
	AddSoundToSequenceTable();
	AddSoundToAWBStream(FolderPath + "\\");


	doc.SaveFile(ACBpath.c_str());
}

int AddSound2ACB::GetAddedFiles(const std::string& FolderPath)
{
	
	for (const auto& entry : std::filesystem::directory_iterator(FolderPath)) {
		if (entry.is_regular_file()) {
			auto extension = entry.path().extension().string();
			if (extension == ".hca") {
				FileToAdd_t out;
				out.FileName = entry.path().filename().string();
				out.FileStem = entry.path().stem().string();
				out.IsWAV = 0;
				out.awbID = -1;
				v_File.push_back(out);
			}
			else if (extension == ".wav") {
				FileToAdd_t out;
				out.FileName = entry.path().filename().string();
				out.FileStem = entry.path().stem().string();
				out.IsWAV = 1;
				out.awbID = -1;
				v_File.push_back(out);
				HasWavFile = 1;
			}
			// if end
		}
		// for end
	}

	AddFileCount = v_File.size();
	return AddFileCount;
}

int AddSound2ACB::GetAWBStreamNode(tinyxml2::XMLElement* header)
{
	for (tinyxml2::XMLElement* entry = header->FirstChildElement("AWB"); entry != 0; entry = entry->NextSiblingElement("AWB")) {
		int isStream = entry->IntAttribute("isStream", 0);

		if (isStream) {
			xml_AWBStream = entry;

			UTF_AWBStream_t out;
			for (tinyxml2::XMLElement* entry2 = entry->FirstChildElement("cue"); entry2 != 0; entry2 = entry2->NextSiblingElement("cue")) {
				int CueID = entry2->IntAttribute("ID", -1);
				/*if (CueID > MaxAWBCueId) {
					MaxAWBCueId = CueID;
				}*/

				out.CueID = CueID;
				out.CueFile = entry2->Attribute("File");
				v_AWBStream.push_back(out);
			}

			if (InsertMode) {
				xml_AWBStream->DeleteChildren();
			}

			return 0;
		}
	}

	return 1;
}

AddSound2ACB::NodeErrorCode AddSound2ACB::GetUTFNode(tinyxml2::XMLElement* mainData)
{
	for (tinyxml2::XMLElement* entry = mainData->FirstChildElement(); entry != 0; entry = entry->NextSiblingElement()) {
		if (!strcmp(entry->Attribute("name"), "CueTable")) {
			xml_CueTable = entry;
		}
		else if (!strcmp(entry->Attribute("name"), "CueNameTable")) {
			xml_CueNameTable = entry;
		}
		else if (!strcmp(entry->Attribute("name"), "WaveformTable")) {
			xml_WaveformTable = entry;
		}
		else if (!strcmp(entry->Attribute("name"), "SynthTable")) {
			xml_SynthTable = entry;
		}
		else if (!strcmp(entry->Attribute("name"), "TrackTable")) {
			xml_TrackTable = entry;
		}
		else if (!strcmp(entry->Attribute("name"), "TrackEventTable")) {
			xml_TrackEventTable = entry;
		}
		else if (!strcmp(entry->Attribute("name"), "SequenceTable")) {
			xml_SequenceTable = entry;
		}
	}

	// check required nodes
	{
		if (!xml_CueTable) {
			return NodeErrorCode::NoCueTable;
		}

		if (!xml_CueNameTable) {
			return NodeErrorCode::NoCueNameTable;
		}

		if (!xml_WaveformTable) {
			return NodeErrorCode::NoWaveformTable;
		}

		if (!xml_SynthTable) {
			return NodeErrorCode::NoSynthTable;
		}

		if (!xml_TrackTable) {
			return NodeErrorCode::NoTrackTable;
		}

		if (!xml_TrackEventTable) {
			return NodeErrorCode::NoTrackEventTable;
		}

		if (!xml_SequenceTable) {
			return NodeErrorCode::NoSequenceTable;
		}
	}


	tinyxml2::XMLNode* xmlNode;
	// Read CueNameTable
	{
		UTF_CueNameTable_t out;
		xmlNode = xml_CueNameTable->FirstChildElement("node");
		while (xmlNode) {
			auto xmlData = xmlNode;
			xmlNode = xmlNode->NextSiblingElement("node");

			out.CueName = GetNamedNodeValue_Str(xmlData, "CueName");
			int CueIndex = GetNamedNodeValue_Uint(xmlData, "CueIndex");
			if (CueIndex > MaxCueIndex) {
				MaxCueIndex = CueIndex;
			}

			out.CueIndex = CueIndex;
			v_CueNameTable.push_back(out);

			xml_CueNameTable->DeleteChild(xmlData);
		}
	}

	// Read CueTable
	{
		UTF_CueTable_t out;
		xmlNode = xml_CueTable->FirstChildElement("node");
		while (xmlNode) {
			auto xmlData = xmlNode;
			xmlNode = xmlNode->NextSiblingElement("node");

			int CueId = GetNamedNodeValue_Uint(xmlData, "CueId");
			if (CueId > MaxCueId) {
				MaxCueId = CueId;
			}

			int ReferenceIndex = GetNamedNodeValue_Uint(xmlData, "ReferenceIndex");
			if (ReferenceIndex > MaxReferenceIndex) {
				MaxReferenceIndex = ReferenceIndex;
			}

			if (InsertMode) {
				out.CueId = CueId;
				out.ReferenceIndex = ReferenceIndex;
				out.Length = GetNamedNodeValue_Uint(xmlData, "Length");

				v_CueTable.push_back(out);
				xml_CueTable->DeleteChild(xmlData);
			}
		}
	}

	// Read WaveformTable
	{
		UTF_WaveformTable_t out;
		xmlNode = xml_WaveformTable->FirstChildElement("node");
		while (xmlNode) {
			auto xmlData = xmlNode;
			xmlNode = xmlNode->NextSiblingElement("node");

			// they are not needed at the moment.
			/*auto MemoryAwbId = GetNamedNodeValue_Uint(xmlData, "MemoryAwbId");
			if (MemoryAwbId > MaxMemoryAwbId) {
				MaxMemoryAwbId = MemoryAwbId;
			}
			out.EncodeType = GetNamedNodeValue_Uint(xmlData, "EncodeType");
			out.Streaming = GetNamedNodeValue_Uint(xmlData, "Streaming");
			out.MemoryAwbId = MemoryAwbId;*/

			int StreamAwbId = GetNamedNodeValue_Uint(xmlData, "StreamAwbId");
			if (StreamAwbId > MaxStreamAwbId) {
				MaxStreamAwbId = StreamAwbId;
			}

			if (InsertMode) {
				out.StreamAwbId = StreamAwbId;
				out.LoopFlag = GetNamedNodeValue_Uint(xmlData, "LoopFlag");
				out.NumSamples = GetNamedNodeValue_Uint(xmlData, "NumSamples");
				out.ExtensionData = GetNamedNodeValue_Uint(xmlData, "ExtensionData");

				v_WaveformTable.push_back(out);
				xml_WaveformTable->DeleteChild(xmlData);
			}
		}
	}

	// Read SynthTable
	if (InsertMode) {
		UTF_SynthTable_t out;
		xmlNode = xml_SynthTable->FirstChildElement("node");
		while (xmlNode) {
			auto xmlData = xmlNode;
			xmlNode = xmlNode->NextSiblingElement("node");

			int ControlWorkArea1 = GetNamedNodeValue_Uint(xmlData, "ControlWorkArea1");
			if (ControlWorkArea1 > MaxSynthControl) {
				MaxSynthControl = ControlWorkArea1;
			}

			auto xml_items = GetNamedNode(xmlData, "ReferenceItems");
			if (xml_items) {
				auto buffer = HexStringToRawData(xml_items->GetText());
				if (buffer.size() >= 4) {
					out.ReferenceItems = ReadUINT16(&buffer[2], 1);
				}
				else {
					out.ReferenceItems = -1;
				}
			}
			else {
				out.ReferenceItems = -1;
			}

			out.CommandIndex = GetNamedNodeValue_Uint(xmlData, "CommandIndex");
			out.ControlWorkArea1 = ControlWorkArea1;
			out.ControlWorkArea2 = GetNamedNodeValue_Uint(xmlData, "ControlWorkArea2");
			v_SynthTable.push_back(out);
			xml_SynthTable->DeleteChild(xmlData);
		}
	}

	// Read TrackTable
	{
		UTF_TrackTable_t out;
		xmlNode = xml_TrackTable->FirstChildElement("node");
		while (xmlNode) {
			auto xmlData = xmlNode;
			xmlNode = xmlNode->NextSiblingElement("node");

			int EventIndex = GetNamedNodeValue_Uint(xmlData, "EventIndex");
			if (EventIndex > MaxEventIndex) {
				MaxEventIndex = EventIndex;
			}

			if (InsertMode) {
				out.EventIndex = EventIndex;
				out.CommandIndex = GetNamedNodeValue_Uint(xmlData, "CommandIndex");
				v_TrackTable.push_back(out);
				xml_TrackTable->DeleteChild(xmlData);
			}
		}
	}

	// Read TrackEventTable
	if (InsertMode) {
		xmlNode = xml_TrackEventTable->FirstChildElement("node");

		while (xmlNode) {
			auto xmlData = xmlNode;
			xmlNode = xmlNode->NextSiblingElement("node");

			auto xmlCMD = xmlData->FirstChildElement("var");
			if (xmlCMD) {
				auto opcode = xmlCMD->FirstChildElement("cmd");

				int waveId = opcode->UnsignedAttribute("waveId", -1);
				if (waveId > MaxWaveId) {
					MaxWaveId = waveId;
				}

				v_TrackEventTable.push_back(waveId);
				xml_TrackEventTable->DeleteChild(xmlData);
			}
		}
	}

	// Read SequenceTable
	if (InsertMode) {
		UTF_SequenceTable_t out;
		xmlNode = xml_SequenceTable->FirstChildElement("node");
		while (xmlNode) {
			auto xmlData = xmlNode;
			xmlNode = xmlNode->NextSiblingElement("node");

			int ControlWorkArea1 = GetNamedNodeValue_Uint(xmlData, "ControlWorkArea1");
			if (ControlWorkArea1 > MaxSequenceControl) {
				MaxSequenceControl = ControlWorkArea1;
			}

			auto TrackIndex = GetNamedNode(xmlData, "TrackIndex");
			const char* trackIndexText = nullptr;
			if (TrackIndex) {
				trackIndexText = TrackIndex->GetText();
				auto buffer = HexStringToRawData(trackIndexText);
				if (buffer.size() >= 2) {
					out.i_TrackIndex = ReadUINT16(&buffer[0], 1);
				}
				else {
					out.i_TrackIndex = -1;
				}
			}
			else {
				trackIndexText = "";
				out.i_TrackIndex = -1;
			}

			out.TrackIndex = trackIndexText;
			out.CommandIndex = GetNamedNodeValue_Uint(xmlData, "CommandIndex");
			out.ControlWorkArea1 = ControlWorkArea1;
			out.ControlWorkArea2 = GetNamedNodeValue_Uint(xmlData, "ControlWorkArea2");
			v_SequenceTable.push_back(out);
			xml_SequenceTable->DeleteChild(xmlData);
		}
	}

	return NodeErrorCode::NoError;
}

tinyxml2::XMLElement* AddSound2ACB::GetNamedNode(tinyxml2::XMLNode* xmlData, const char* name)
{
	tinyxml2::XMLElement* node = xmlData->FirstChildElement();
	while (node) {
		if (!strcmp(node->Attribute("name"), name)) {
			break;
		}
		node = node->NextSiblingElement();
	}

	return node;
}

const char* AddSound2ACB::GetNamedNodeValue_Str(tinyxml2::XMLNode* xmlData, const char* name)
{
	auto node = GetNamedNode(xmlData, name);
	if (node) {
		return node->Attribute("value");
	} else {
		return nullptr;
	}
}

UINT32 AddSound2ACB::GetNamedNodeValue_Uint(tinyxml2::XMLNode* xmlData, const char* name)
{
	auto node = GetNamedNode(xmlData, name);
	if (node) {
		return node->UnsignedAttribute("value", -1);
	}
	else {
		return -1;
	}
}

void AddSound2ACB::UpdateDataInInsertMode()
{

	for (int i = 0; i < v_CueNameTable.size(); i++) {
		if (v_CueNameTable[i].CueIndex == MaxCueIndex) {
			v_CueNameTable[i].CueIndex += AddFileCount;
			break;
		}
	}
	MaxCueIndex -= 1;

	v_CueTable.back().CueId += AddFileCount;
	v_CueTable.back().ReferenceIndex += AddFileCount;
	MaxCueId -= 1;
	MaxReferenceIndex -= 1;

	// It's special because sometimes it's not last.
	int WaveformTableSize = v_WaveformTable.size();
	for (int i = 0; i < WaveformTableSize; i++) {
		if (v_WaveformTable[i].StreamAwbId == MaxStreamAwbId) {
			v_WaveformTable[i].StreamAwbId += AddFileCount;
			break;
		}
	}
	MaxStreamAwbId -= 1;

	v_SynthTable.back().ReferenceItems += AddFileCount;
	v_SynthTable.back().ControlWorkArea1 += AddFileCount;
	v_SynthTable.back().ControlWorkArea2 += AddFileCount;
	MaxSynthControl -= 1;

	v_TrackTable.back().EventIndex += AddFileCount;
	MaxEventIndex -= 1;

	v_TrackEventTable.back() += AddFileCount;
	MaxTrackIndex = v_TrackTable.size() - 1;

	v_SequenceTable.back().i_TrackIndex += AddFileCount;
	v_SequenceTable.back().ControlWorkArea1 += AddFileCount;
	v_SequenceTable.back().ControlWorkArea2 += AddFileCount;
	MaxSequenceControl -= 1;

	v_AWBStream.back().CueID += AddFileCount;
	MaxWaveId -= 1;


	// update last one directly, because there's no need for sorting.
	/*
	for (int i = 0; i < v_CueTable.size(); i++) {
		if (v_CueTable[i].CueId == MaxCueId) {
			v_CueTable[i].CueId += AddFileCount;
		}

		if (v_CueTable[i].ReferenceIndex == MaxCueIndex) {
			v_CueTable[i].ReferenceIndex += AddFileCount;
		}
	}

	for (int i = 0; i < v_SynthTable.size(); i++) {
		if (v_SynthTable[i].ReferenceItems == WaveformTableSize) {
			v_SynthTable[i].ReferenceItems += AddFileCount;
		}

		if (v_SynthTable[i].ControlWorkArea1 == MaxSynthControl) {
			v_SynthTable[i].ControlWorkArea1 += AddFileCount;
			v_SynthTable[i].ControlWorkArea2 += AddFileCount;
		}
	}

	int TrackTableSize = v_TrackTable.size();
	for (int i = 0; i < TrackTableSize; i++) {
		if (v_TrackTable[i].EventIndex == MaxEventIndex) {
			v_TrackTable[i].EventIndex += AddFileCount;
			break;
		}
	}

	for (int i = 0; i < v_TrackEventTable.size(); i++) {
		if (v_TrackEventTable[i] == MaxWaveId) {
			v_TrackEventTable[i] += AddFileCount;
			break;
		}
	}

	for (int i = 0; i < v_SequenceTable.size(); i++) {
		if (v_SequenceTable[i].i_TrackIndex == TrackTableSize) {
			v_SequenceTable[i].i_TrackIndex += AddFileCount;
		}

		if (v_SequenceTable[i].ControlWorkArea1 == MaxSequenceControl) {
			v_SequenceTable[i].ControlWorkArea1 += AddFileCount;
			v_SequenceTable[i].ControlWorkArea2 += AddFileCount;
		}
	}*/
}

void AddSound2ACB::AddSoundToCueNameTable()
{
	UTF_CueNameTable_t out;
	int CueCount = MaxCueIndex;
	for (int i = 0; i < AddFileCount; i++) {
		CueCount++;
		out.CueIndex = CueCount;
		out.CueName = v_File[i].FileStem;
		v_CueNameTable.push_back(out);
	}

	std::sort(v_CueNameTable.begin(), v_CueNameTable.end());

	for (int i = 0; i < v_CueNameTable.size(); i++) {
		auto xmlNode = xml_CueNameTable->InsertNewChildElement("node");

		auto xml_CueName = xmlNode->InsertNewChildElement("var");
		xml_CueName->SetAttribute("name", "CueName");
		xml_CueName->SetAttribute("value", v_CueNameTable[i].CueName.c_str());

		auto xml_CueIndex = xmlNode->InsertNewChildElement("var");
		xml_CueIndex->SetAttribute("name", "CueIndex");
		xml_CueIndex->SetAttribute("value", v_CueNameTable[i].CueIndex);
	}
}

void AddSound2ACB::AddSoundToCueTable()
{
	if (InsertMode) {
		int size = v_CueTable.size() - 1;
		for (int i = 0; i < size; i++) {
			auto xmlNode = xml_CueTable->InsertNewChildElement("node");

			auto xml_CueId = xmlNode->InsertNewChildElement("var");
			xml_CueId->SetAttribute("name", "CueId");
			xml_CueId->SetAttribute("value", v_CueTable[i].CueId);

			auto xml_ReferenceIndex = xmlNode->InsertNewChildElement("var");
			xml_ReferenceIndex->SetAttribute("name", "ReferenceIndex");
			xml_ReferenceIndex->SetAttribute("value", v_CueTable[i].ReferenceIndex);

			auto xml_Length = xmlNode->InsertNewChildElement("var");
			xml_Length->SetAttribute("name", "Length");
			xml_Length->SetAttribute("value", v_CueTable[i].Length);
		}
	}

	// add new node
	int CueIdCount = MaxCueId;
	int ReferenceIndexCount = MaxReferenceIndex;
	for (int i = 0; i < AddFileCount; i++) {
		CueIdCount++;
		ReferenceIndexCount++;

		auto xmlNode = xml_CueTable->InsertNewChildElement("node");

		auto xml_CueId = xmlNode->InsertNewChildElement("var");
		xml_CueId->SetAttribute("name", "CueId");
		xml_CueId->SetAttribute("value", CueIdCount);

		auto xml_ReferenceIndex = xmlNode->InsertNewChildElement("var");
		xml_ReferenceIndex->SetAttribute("name", "ReferenceIndex");
		xml_ReferenceIndex->SetAttribute("value", ReferenceIndexCount);

		auto xml_Length = xmlNode->InsertNewChildElement("var");
		xml_Length->SetAttribute("name", "Length");
		xml_Length->SetAttribute("value", "4294967295");
	}

	if (InsertMode) {
		auto xmlNode = xml_CueTable->InsertNewChildElement("node");

		auto xml_CueId = xmlNode->InsertNewChildElement("var");
		xml_CueId->SetAttribute("name", "CueId");
		xml_CueId->SetAttribute("value", v_CueTable.back().CueId);

		auto xml_ReferenceIndex = xmlNode->InsertNewChildElement("var");
		xml_ReferenceIndex->SetAttribute("name", "ReferenceIndex");
		xml_ReferenceIndex->SetAttribute("value", v_CueTable.back().ReferenceIndex);

		auto xml_Length = xmlNode->InsertNewChildElement("var");
		xml_Length->SetAttribute("name", "Length");
		xml_Length->SetAttribute("value", v_CueTable.back().Length);
	}
	// end
}

void AddSound2ACB::AddSoundToWaveformTable()
{
	if (InsertMode) {
		int size = v_WaveformTable.size() - 1;
		for (int i = 0; i < size; i++) {
			auto xmlNode = xml_WaveformTable->InsertNewChildElement("node");

			/*auto xml_EncodeType = xmlNode->InsertNewChildElement("var");
			xml_EncodeType->SetAttribute("name", "EncodeType");
			xml_EncodeType->SetAttribute("value", v_WaveformTable[i].EncodeType);
			auto xml_Streaming = xmlNode->InsertNewChildElement("var");
			xml_Streaming->SetAttribute("name", "Streaming");
			xml_Streaming->SetAttribute("value", v_WaveformTable[i].Streaming);*/

			auto xml_LoopFlag = xmlNode->InsertNewChildElement("var");
			xml_LoopFlag->SetAttribute("name", "LoopFlag");
			xml_LoopFlag->SetAttribute("value", v_WaveformTable[i].LoopFlag);

			auto xml_NumSamples = xmlNode->InsertNewChildElement("var");
			xml_NumSamples->SetAttribute("name", "NumSamples");
			xml_NumSamples->SetAttribute("value", v_WaveformTable[i].NumSamples)
				;
			auto xml_ExtensionData = xmlNode->InsertNewChildElement("var");
			xml_ExtensionData->SetAttribute("name", "ExtensionData");
			xml_ExtensionData->SetAttribute("value", v_WaveformTable[i].ExtensionData);

			auto xml_StreamAwbId = xmlNode->InsertNewChildElement("var");
			xml_StreamAwbId->SetAttribute("name", "StreamAwbId");
			xml_StreamAwbId->SetAttribute("value", v_WaveformTable[i].StreamAwbId);
		}
	}

	// add new node
	int StreamAwbIdCount = MaxStreamAwbId;
	for (int i = 0; i < AddFileCount; i++) {
		StreamAwbIdCount++;
		auto xmlNode = xml_WaveformTable->InsertNewChildElement("node");
		auto xml_LoopFlag = xmlNode->InsertNewChildElement("var");
		xml_LoopFlag->SetAttribute("name", "LoopFlag");
		xml_LoopFlag->SetAttribute("value", "2");

		auto xml_NumSamples = xmlNode->InsertNewChildElement("var");
		xml_NumSamples->SetAttribute("name", "NumSamples");
		xml_NumSamples->SetAttribute("value", "4399");

		auto xml_ExtensionData = xmlNode->InsertNewChildElement("var");
		xml_ExtensionData->SetAttribute("name", "ExtensionData");
		xml_ExtensionData->SetAttribute("value", "65535");

		auto xml_StreamAwbId = xmlNode->InsertNewChildElement("var");
		xml_StreamAwbId->SetAttribute("name", "StreamAwbId");
		xml_StreamAwbId->SetAttribute("value", StreamAwbIdCount);
		v_File[i].awbID = StreamAwbIdCount;
	}

	if (InsertMode) {
		auto xmlNode = xml_WaveformTable->InsertNewChildElement("node");

		auto xml_LoopFlag = xmlNode->InsertNewChildElement("var");
		xml_LoopFlag->SetAttribute("name", "LoopFlag");
		xml_LoopFlag->SetAttribute("value", v_WaveformTable.back().LoopFlag);

		auto xml_NumSamples = xmlNode->InsertNewChildElement("var");
		xml_NumSamples->SetAttribute("name", "NumSamples");
		xml_NumSamples->SetAttribute("value", v_WaveformTable.back().NumSamples)
			;
		auto xml_ExtensionData = xmlNode->InsertNewChildElement("var");
		xml_ExtensionData->SetAttribute("name", "ExtensionData");
		xml_ExtensionData->SetAttribute("value", v_WaveformTable.back().ExtensionData);

		auto xml_StreamAwbId = xmlNode->InsertNewChildElement("var");
		xml_StreamAwbId->SetAttribute("name", "StreamAwbId");
		xml_StreamAwbId->SetAttribute("value", v_WaveformTable.back().StreamAwbId);
	}
	// end
}

void AddSound2ACB::AddSoundToSynthTable()
{
	char buffer[4];
	if (InsertMode) {
		auto size = v_SynthTable.size() - 1;
		for (int i = 0; i < size; i++) {
			auto xmlNode = xml_SynthTable->InsertNewChildElement("node");

			auto xml_CommandIndex = xmlNode->InsertNewChildElement("var");
			xml_CommandIndex->SetAttribute("name", "CommandIndex");
			xml_CommandIndex->SetAttribute("value", v_SynthTable[i].CommandIndex);

			auto xml_ReferenceItems = xmlNode->InsertNewChildElement("var");
			xml_ReferenceItems->SetAttribute("name", "ReferenceItems");
			AddSoundToSynthTable_SetText(buffer, v_SynthTable[i].ReferenceItems);
			xml_ReferenceItems->SetAttribute("value", "RAW");
			xml_ReferenceItems->SetText(RawDataToHexString(buffer, 4).c_str());

			auto xml_ControlWorkArea1 = xmlNode->InsertNewChildElement("var");
			xml_ControlWorkArea1->SetAttribute("name", "ControlWorkArea1");
			xml_ControlWorkArea1->SetAttribute("value", v_SynthTable[i].ControlWorkArea1);

			auto xml_ControlWorkArea2 = xmlNode->InsertNewChildElement("var");
			xml_ControlWorkArea2->SetAttribute("name", "ControlWorkArea2");
			xml_ControlWorkArea2->SetAttribute("value", v_SynthTable[i].ControlWorkArea2);
		}
	}

	// add new node
	int SynthControlCount = MaxSynthControl;
	for (int i = 0; i < AddFileCount; i++) {
		SynthControlCount++;
		auto xmlNode = xml_SynthTable->InsertNewChildElement("node");

		auto xml_CommandIndex = xmlNode->InsertNewChildElement("var");
		xml_CommandIndex->SetAttribute("name", "CommandIndex");
		xml_CommandIndex->SetAttribute("value", "Modify it to SynthTable need");

		auto xml_ReferenceItems = xmlNode->InsertNewChildElement("var");
		xml_ReferenceItems->SetAttribute("name", "ReferenceItems");
		xml_ReferenceItems->SetAttribute("value", "RAW");
		AddSoundToSynthTable_SetText(buffer, SynthControlCount);
		xml_ReferenceItems->SetText(RawDataToHexString(buffer, 4).c_str());

		auto xml_ControlWorkArea1 = xmlNode->InsertNewChildElement("var");
		xml_ControlWorkArea1->SetAttribute("name", "ControlWorkArea1");
		xml_ControlWorkArea1->SetAttribute("value", SynthControlCount);
		auto xml_ControlWorkArea2 = xmlNode->InsertNewChildElement("var");
		xml_ControlWorkArea2->SetAttribute("name", "ControlWorkArea2");
		xml_ControlWorkArea2->SetAttribute("value", SynthControlCount);
	}

	if (InsertMode) {
		auto xmlNode = xml_SynthTable->InsertNewChildElement("node");

		auto xml_CommandIndex = xmlNode->InsertNewChildElement("var");
		xml_CommandIndex->SetAttribute("name", "CommandIndex");
		xml_CommandIndex->SetAttribute("value", v_SynthTable.back().CommandIndex);

		auto xml_ReferenceItems = xmlNode->InsertNewChildElement("var");
		xml_ReferenceItems->SetAttribute("name", "ReferenceItems");
		xml_ReferenceItems->SetAttribute("value", "RAW");
		AddSoundToSynthTable_SetText(buffer, v_SynthTable.back().ReferenceItems);
		xml_ReferenceItems->SetText(RawDataToHexString(buffer, 4).c_str());

		auto xml_ControlWorkArea1 = xmlNode->InsertNewChildElement("var");
		xml_ControlWorkArea1->SetAttribute("name", "ControlWorkArea1");
		xml_ControlWorkArea1->SetAttribute("value", v_SynthTable.back().ControlWorkArea1);

		auto xml_ControlWorkArea2 = xmlNode->InsertNewChildElement("var");
		xml_ControlWorkArea2->SetAttribute("name", "ControlWorkArea2");
		xml_ControlWorkArea2->SetAttribute("value", v_SynthTable.back().ControlWorkArea2);
	}
	// end
}

void AddSound2ACB::AddSoundToSynthTable_SetText(char* buf, int in)
{
	buf[0] = 0;
	buf[1] = 1;
	WriteINT16BE(&buf[2], in);
}

void AddSound2ACB::AddSoundToTrackTable()
{
	if (InsertMode) {
		auto size = v_TrackTable.size() - 1;
		for (int i = 0; i < size; i++) {
			auto xmlNode = xml_TrackTable->InsertNewChildElement("node");

			auto xml_EventIndex = xmlNode->InsertNewChildElement("var");
			xml_EventIndex->SetAttribute("name", "EventIndex");
			xml_EventIndex->SetAttribute("value", v_TrackTable[i].EventIndex);

			auto xml_CommandIndex = xmlNode->InsertNewChildElement("var");
			xml_CommandIndex->SetAttribute("name", "CommandIndex");
			xml_CommandIndex->SetAttribute("value", v_TrackTable[i].CommandIndex);
		}
	}

	// add new node
	int EventIndexCount = MaxEventIndex;
	for (int i = 0; i < AddFileCount; i++) {
		EventIndexCount++;
		auto xmlNode = xml_TrackTable->InsertNewChildElement("node");

		auto xml_EventIndex = xmlNode->InsertNewChildElement("var");
		xml_EventIndex->SetAttribute("name", "EventIndex");
		xml_EventIndex->SetAttribute("value", EventIndexCount);

		auto xml_CommandIndex = xmlNode->InsertNewChildElement("var");
		xml_CommandIndex->SetAttribute("name", "CommandIndex");
		xml_CommandIndex->SetAttribute("value", "Modify it to TrackTable need");
	}

	if (InsertMode) {
		auto xmlNode = xml_TrackTable->InsertNewChildElement("node");

		auto xml_EventIndex = xmlNode->InsertNewChildElement("var");
		xml_EventIndex->SetAttribute("name", "EventIndex");
		xml_EventIndex->SetAttribute("value", v_TrackTable.back().EventIndex);

		auto xml_CommandIndex = xmlNode->InsertNewChildElement("var");
		xml_CommandIndex->SetAttribute("name", "CommandIndex");
		xml_CommandIndex->SetAttribute("value", v_TrackTable.back().CommandIndex);
	}
	// end
}

void AddSound2ACB::AddSoundToTrackEventTable()
{
	if (InsertMode) {
		auto size = v_TrackEventTable.size() - 1;
		for (int i = 0; i < size; i++) {
			auto xmlNode = xml_TrackEventTable->InsertNewChildElement("node");

			AddSoundToTrackEventTable_SetNode(xmlNode, v_TrackEventTable[i]);
		}
	}

	// add new node
	int WaveIdCount = MaxWaveId;
	for (int i = 0; i < AddFileCount; i++) {
		WaveIdCount++;
		auto xmlNode = xml_TrackEventTable->InsertNewChildElement("node");
		AddSoundToTrackEventTable_SetNode(xmlNode, WaveIdCount);
	}

	if (InsertMode) {
		auto xmlNode = xml_TrackEventTable->InsertNewChildElement("node");
		AddSoundToTrackEventTable_SetNode(xmlNode, v_TrackEventTable.back());
	}
	// end
}

void AddSound2ACB::AddSoundToTrackEventTable_SetNode(tinyxml2::XMLElement* xmlNode, int i)
{
	auto xml_CMD = xmlNode->InsertNewChildElement("var");
	xml_CMD->SetAttribute("name", "Command");
	xml_CMD->SetAttribute("value", "CMD");

	auto xml_opcode1 = xml_CMD->InsertNewChildElement("cmd");
	xml_opcode1->SetAttribute("code", "2000");
	xml_opcode1->SetAttribute("wavetype", "2");
	xml_opcode1->SetAttribute("waveId", i);

	auto xml_opcode2 = xml_CMD->InsertNewChildElement("opcode");
	xml_opcode2->SetAttribute("type", "0");
}

void AddSound2ACB::AddSoundToSequenceTable()
{
	char buffer[2];

	if (InsertMode) {
		auto size = v_SequenceTable.size() - 1;
		for (int i = 0; i < size; i++) {
			auto xmlNode = xml_SequenceTable->InsertNewChildElement("node");

			auto xml_TrackIndex = xmlNode->InsertNewChildElement("var");
			xml_TrackIndex->SetAttribute("name", "TrackIndex");
			xml_TrackIndex->SetAttribute("value", "RAW");
			WriteINT16BE(buffer, v_SequenceTable[i].i_TrackIndex);
			xml_TrackIndex->SetText(RawDataToHexString(buffer, 2).c_str());

			auto xml_CommandIndex = xmlNode->InsertNewChildElement("var");
			xml_CommandIndex->SetAttribute("name", "CommandIndex");
			xml_CommandIndex->SetAttribute("value", v_SequenceTable[i].CommandIndex);

			auto xml_ControlWorkArea1 = xmlNode->InsertNewChildElement("var");
			xml_ControlWorkArea1->SetAttribute("name", "ControlWorkArea1");
			xml_ControlWorkArea1->SetAttribute("value", v_SequenceTable[i].ControlWorkArea1);
			auto xml_ControlWorkArea2 = xmlNode->InsertNewChildElement("var");
			xml_ControlWorkArea2->SetAttribute("name", "ControlWorkArea2");
			xml_ControlWorkArea2->SetAttribute("value", v_SequenceTable[i].ControlWorkArea2);
		}
	}

	// add new node
	int TrackIndexCount = MaxTrackIndex;
	for (int i = 0; i < AddFileCount; i++) {
		TrackIndexCount++;
		auto xmlNode = xml_SequenceTable->InsertNewChildElement("node");

		auto xml_TrackIndex = xmlNode->InsertNewChildElement("var");
		xml_TrackIndex->SetAttribute("name", "TrackIndex");
		xml_TrackIndex->SetAttribute("value", "RAW");
		WriteINT16BE(buffer, TrackIndexCount);
		xml_TrackIndex->SetText(RawDataToHexString(buffer, 2).c_str());

		auto xml_CommandIndex = xmlNode->InsertNewChildElement("var");
		xml_CommandIndex->SetAttribute("name", "CommandIndex");
		xml_CommandIndex->SetAttribute("value", "Modify it to SequenceTable need");

		auto xml_ControlWorkArea1 = xmlNode->InsertNewChildElement("var");
		xml_ControlWorkArea1->SetAttribute("name", "ControlWorkArea1");
		xml_ControlWorkArea1->SetAttribute("value", TrackIndexCount);
		auto xml_ControlWorkArea2 = xmlNode->InsertNewChildElement("var");
		xml_ControlWorkArea2->SetAttribute("name", "ControlWorkArea2");
		xml_ControlWorkArea2->SetAttribute("value", TrackIndexCount);
	}

	if (InsertMode) {
		auto xmlNode = xml_SequenceTable->InsertNewChildElement("node");

		auto xml_TrackIndex = xmlNode->InsertNewChildElement("var");
		xml_TrackIndex->SetAttribute("name", "TrackIndex");
		xml_TrackIndex->SetAttribute("value", "RAW");
		WriteINT16BE(buffer, v_SequenceTable.back().i_TrackIndex);
		xml_TrackIndex->SetText(RawDataToHexString(buffer, 2).c_str());

		auto xml_CommandIndex = xmlNode->InsertNewChildElement("var");
		xml_CommandIndex->SetAttribute("name", "CommandIndex");
		xml_CommandIndex->SetAttribute("value", v_SequenceTable.back().CommandIndex);

		auto xml_ControlWorkArea1 = xmlNode->InsertNewChildElement("var");
		xml_ControlWorkArea1->SetAttribute("name", "ControlWorkArea1");
		xml_ControlWorkArea1->SetAttribute("value", v_SequenceTable.back().ControlWorkArea1);
		auto xml_ControlWorkArea2 = xmlNode->InsertNewChildElement("var");
		xml_ControlWorkArea2->SetAttribute("name", "ControlWorkArea2");
		xml_ControlWorkArea2->SetAttribute("value", v_SequenceTable.back().ControlWorkArea2);
	}
	// end
}

void AddSound2ACB::AddSoundToAWBStream(const std::string& FolderPath)
{
	int wavIndex = 0;

	if (InsertMode) {
		auto size = v_AWBStream.size() - 1;
		for (int i = 0; i < size; i++) {
			auto xmlNode = xml_AWBStream->InsertNewChildElement("cue");
			xmlNode->SetAttribute("ID", v_AWBStream[i].CueID);
			xmlNode->SetAttribute("File", v_AWBStream[i].CueFile.c_str());
		}
		wavIndex = size - 1;
	} else {
		wavIndex = v_AWBStream.size() - 1;
	}

	// add new node
	int renameWav = 0;
	if (HasWavFile) {
		std::cout << "Rename WAV files to index (0 is no, 1 is yes): ";
		std::cin >> renameWav;
		std::cout << "\n";
	}

	std::string fileLog = "";
	for (int i = 0; i < AddFileCount; i++) {
		auto awbID = v_File[i].awbID;
		if (awbID > -1) {
			wavIndex++;

			auto xmlNode = xml_AWBStream->InsertNewChildElement("cue");
			xmlNode->SetAttribute("ID", awbID);
			if (v_File[i].IsWAV) {
				if (renameWav) {
					auto newName = std::format("{:04d}.wav", wavIndex);
					//std::cout << wavIndex + " " + newName + "\n";
					fileLog += std::to_string(wavIndex) + " " + v_File[i].FileStem + "\n";

					auto oldPath = FolderPath + v_File[i].FileName;
					auto newPath = FolderPath + newName;
					std::filesystem::rename(oldPath, newPath);

					xmlNode->SetAttribute("File", v_AWBStream.back().CueFile.c_str());
					continue;
				}
				// end if
			}

			xmlNode->SetAttribute("File", v_File[i].FileName.c_str());
		}
	}
	// end

	if (InsertMode) {
		auto xmlNode = xml_AWBStream->InsertNewChildElement("cue");
		xmlNode->SetAttribute("ID", v_AWBStream.back().CueID);
		xmlNode->SetAttribute("File", v_AWBStream.back().CueFile.c_str());
	}

	if (renameWav) {
		//system("pause");
		std::ofstream newFile(FolderPath + "mapping.txt", std::ios::binary | std::ios::out | std::ios::ate);
		newFile << fileLog;
		newFile.close();
	}
	// end
}
