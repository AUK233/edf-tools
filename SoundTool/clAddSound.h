#pragma once
#include <unordered_map>
#include "..\EDF_Tools\include\tinyxml2.h"
#include "clUTFstructure.hpp"

class AddSound2ACB {
public:
	enum class NodeErrorCode {
		NoError,
		NoCueTable,
		NoCueNameTable,
		NoWaveformTable,
		NoSynthTable,
		NoTrackTable,
		NoTrackEventTable,
		NoSequenceTable,
	};

	struct FileToAdd_t {
		std::string FileName;
		std::string FileStem; // no extension
		int IsWAV;
		int awbID;
	};

	void AddSound(const std::string& ACBpath, const std::string& FolderPath, const int IsInsert);

	int GetAddedFiles(const std::string& FolderPath);
	int GetAWBStreamNode(tinyxml2::XMLElement* header);
	NodeErrorCode GetUTFNode(tinyxml2::XMLElement* mainData);
	tinyxml2::XMLElement* GetNamedNode(tinyxml2::XMLNode* xmlData, const char* name);
	const char* GetNamedNodeValue_Str(tinyxml2::XMLNode* xmlData, const char* name);
	UINT32 GetNamedNodeValue_Uint(tinyxml2::XMLNode* xmlData, const char* name);

	void UpdateDataInInsertMode();
	void AddSoundToCueNameTable();
	void AddSoundToCueTable();
	void AddSoundToWaveformTable();
	void AddSoundToSynthTable();
	void AddSoundToSynthTable_SetText(char* buf, int in);
	void AddSoundToTrackTable();
	void AddSoundToTrackEventTable();
	void AddSoundToTrackEventTable_SetNode(tinyxml2::XMLElement* xmlNode, int i);
	void AddSoundToSequenceTable();
	void AddSoundToAWBStream(const std::string& FolderPath);

	int InsertMode;
	int HasWavFile = 0;
	int AddFileCount = 0;
	int MaxCueId = 0;
	int MaxReferenceIndex = 0;
	int MaxCueIndex = 0;
	int MaxMemoryAwbId = 0;
	int MaxStreamAwbId = 0;
	int MaxSynthControl = 0;
	int MaxEventIndex = 0;
	int MaxWaveId = 0;
	int MaxTrackIndex = 0;
	int MaxSequenceControl = 0;

	std::vector<FileToAdd_t> v_File;

	tinyxml2::XMLElement* xml_CueTable = 0;
	std::vector<UTF_CueTable_t> v_CueTable;

	tinyxml2::XMLElement* xml_CueNameTable = 0;
	std::vector<UTF_CueNameTable_t> v_CueNameTable;

	tinyxml2::XMLElement* xml_WaveformTable = 0;
	std::vector<UTF_WaveformTable_t> v_WaveformTable;

	tinyxml2::XMLElement* xml_SynthTable = 0;
	std::vector<UTF_SynthTable_t> v_SynthTable;

	tinyxml2::XMLElement* xml_TrackTable = 0;
	std::vector<UTF_TrackTable_t> v_TrackTable;

	tinyxml2::XMLElement* xml_TrackEventTable = 0;
	std::vector<int> v_TrackEventTable;

	tinyxml2::XMLElement* xml_SequenceTable = 0;
	std::vector<UTF_SequenceTable_t> v_SequenceTable;

	tinyxml2::XMLElement* xml_AWBStream = 0;
	std::vector<UTF_AWBStream_t> v_AWBStream;
};
