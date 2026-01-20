#pragma once

struct UTF_CueTable_t {
	int CueId, ReferenceIndex;
	UINT32 Length;
};

struct UTF_CueNameTable_t {
	std::string CueName;
	int CueIndex;

	bool operator<(const UTF_CueNameTable_t& other) const {
		return CueName < other.CueName;
	}
};

struct UTF_WaveformTable_t {
	int EncodeType, Streaming;
	int MemoryAwbId, StreamAwbId;
	int LoopFlag, NumSamples, ExtensionData;
};

struct UTF_SynthTable_t {
	int CommandIndex;
	int ReferenceItems; // this is WaveformTable index, now uses int.
	int ControlWorkArea1, ControlWorkArea2;
};

struct UTF_TrackTable_t {
	int EventIndex, CommandIndex;
};

struct UTF_SequenceTable_t {
	std::string TrackIndex;
	int CommandIndex, i_TrackIndex;
	int ControlWorkArea1, ControlWorkArea2;
};

struct UTF_AWBStream_t {
	std::string CueFile;
	int CueID;
};
