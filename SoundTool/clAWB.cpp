#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>

#include "clAWB.h"
#include "clUtil.h"

void AWB::Read(const std::string& inPath)
{
	// read awb
	std::string tempPath = inPath + ".awb";
	if (!std::filesystem::exists(tempPath)) {
		std::cout << "There is no AWB file!\n";
		return;
	}
	std::ifstream file(tempPath, std::ios::binary | std::ios::ate | std::ios::in);
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	std::vector<char> buffer(size);

	int ReadFileState = 0;
	if (file.read(buffer.data(), size)) {
		ReadFileState = 1;
	}
	file.close();

	// read awe
	tempPath = inPath + ".awe";
	if (!std::filesystem::exists(tempPath)) {
		std::cout << "There is no AWE file!\n";
		return;
	}
	std::ifstream awe_file(tempPath, std::ios::binary | std::ios::ate | std::ios::in);
	std::streamsize awe_size = awe_file.tellg();
	awe_file.seekg(0, std::ios::beg);
	std::vector<char> awe_buffer(awe_size);
	if (awe_file.read(awe_buffer.data(), awe_size)) {
		ReadFileState += 1;
	}
	awe_file.close();

	// check
	if (ReadFileState < 2) {
		return;
	}

	// AWB
	// get header
	memcpy(&v_header, &buffer[0], 0x10);
	// AFS2
	if (v_header.header != 844318273) {
		std::cout << "This is not an AWB file!\n";
		return;
	}
	// get data offset
	ReadDataOffset(buffer);

	// AWE
	// get header
	memcpy(&v_AWEheader, &awe_buffer[0], 20);
	// AWBE
	if (v_AWEheader.header != 1161975617) {
		std::cout << "This is not an AWE file!\n";
		return;
	}
	std::cout << "Reading AWE filename list......\n";
	ReadNameTable(awe_buffer);

	// out file
	if (v_NameTable.size()) {
		std::cout << "Reading AWB data file......\n";
		ReadDataToFile(inPath, buffer);
	}
	v_DataFile.clear();
	v_Name.clear();
	v_NameTable.clear();

	std::cout << "Done!\n";
}

void AWB::ReadDataOffset(const std::vector<char>& buffer)
{
	int base_ofs = 0x10 + (v_header.sound_count * v_header.cueID_size);

	int newSoundCount = v_header.sound_count - 1;
	for (int i = 0; i <= newSoundCount; i++) {
		FileOffset_t data;

		int CueID_offset = 0x10 + (i * v_header.cueID_size);
		data.cueID = ReadUINT16LE(&buffer[CueID_offset]);

		int cur_ofs = base_ofs + (i * v_header.dataOfs_size);
		switch (v_header.dataOfs_size) {
		case 4: /* common */
			data.self = ReadINT32LE(&buffer[cur_ofs]);
			data.next = ReadINT32LE(&buffer[cur_ofs+4]);
			break;
		case 2: /* mostly sfx in .acb */
			data.self = ReadUINT16LE(&buffer[cur_ofs]);
			data.next = ReadUINT16LE(&buffer[cur_ofs+2]);
			break;
		default:
			std::cout << "AWB: unknown offset size\n";
			return;
		}

		// offset are absolute but sometimes misaligned (specially first that just points to offset table end)
		int alignmentOfs = v_header.align_size;
		int alignmentPos = data.self % alignmentOfs;
		if (alignmentPos) {
			data.self += alignmentOfs - alignmentPos;
		}

		if (i < newSoundCount) {
			alignmentPos = data.next % alignmentOfs;
			if (alignmentPos) {
				data.next += alignmentOfs - alignmentPos;
			}
		}
		else {
			data.next = buffer.size();
		}

		v_DataFile.push_back(data);
	}
}

void AWB::ReadNameTable(const std::vector<char>& buffer)
{
	// get name
	for (int i = 0; i < v_AWEheader.sound_count; i++) {
		int curPos = v_AWEheader.nameTable_offset + (i * 4);

		int strOfs = ReadINT32LE(&buffer[curPos]);
		strOfs += curPos;

		std::string fileName = &buffer[strOfs];
		v_Name.push_back(fileName);
	}
	// get index
	for (int i = 0; i < v_AWEheader.sound_count; i++) {
		int curPos = v_AWEheader.fileTable_offset + (i * 2);
		v_NameTable.push_back(ReadUINT16LE(&buffer[curPos]));
	}

	std::unordered_map<int, int> CueMap;
	for (int i = 0; i < v_DataFile.size(); i++) {
		CueMap[v_DataFile[i].cueID] = i;
	}

	for (int i = 0; i < v_NameTable.size(); i++) {
		if (CueMap.find(v_NameTable[i]) != CueMap.end()) {
			v_NameTable[i] = CueMap[v_NameTable[i]];
		}
	}
}

void AWB::ReadDataToFile(const std::string& inPath, const std::vector<char>& buffer)
{
	int FolderExists = DirectoryExists(inPath.c_str());
	if (!FolderExists) {
		CreateDirectoryA(inPath.c_str(), NULL);
	}

	for (int i = 0; i < v_NameTable.size(); i++) {
		int AWBIndex = v_NameTable[i];

		int selfOfs = v_DataFile[AWBIndex].self;
		int nextOfs = v_DataFile[AWBIndex].next;
		std::string fileName = v_Name[i];

		std::vector<char> v_data(buffer.begin() + selfOfs, buffer.begin() + nextOfs);
		std::ofstream newFile(inPath + "\\" + fileName + ".hca", std::ios::binary | std::ios::out | std::ios::ate);
		newFile.write(v_data.data(), v_data.size());
		newFile.close();
	}
}

void AWB::Write(const std::string& inPath)
{
	if (!std::filesystem::is_directory(inPath)) {
		std::cout << "Here a folder is necessary!\n";
		return;
	}

	// initialization data
	WriteInitHeader();
	// capacity increase to UINT16 max.
	v_File.reserve(0xFFFF);
	// 
	int i_DataSize = 0;
	int i_NameLength = 0;

	// read file
	std::cout << "Getting HCA files:\n0";
	for (const auto& entry : std::filesystem::directory_iterator(inPath)) {
		if (entry.is_regular_file()) {
			if (entry.path().extension().string() == ".hca") {
				//std::cout << entry.path().filename().string() + "\n";
				//std::cout << entry.path().stem().string() + "\n";
				//std::cout << entry.path().generic_string() + "\n";
				PreprocessFileData_t out;

				std::ifstream file(entry.path().generic_string(), std::ios::binary | std::ios::ate | std::ios::in);
				std::streamsize size = file.tellg();
				file.seekg(0, std::ios::beg);
				out.data.resize(size);
				if (file.read(out.data.data(), size)) {
					file.close();

					// set value
					int tempIndex = v_File.size();
					out.index = tempIndex;
					out.CueID = tempIndex;
					out.name = entry.path().stem().string();
					out.offset_data = i_DataSize;
					out.offset_name = i_NameLength;

					// update value
					i_NameLength += out.name.size()+1;

					// 32-byte alignment in here
					int align_buffer = out.data.size() % 32;
					if (align_buffer) {
						for (int i = align_buffer; i < 32; i++) {
							out.data.push_back(0);
						}
					}
					i_DataSize += out.data.size();

					v_File.push_back(out);
					// show progress
					std::cout << "\r" + std::to_string(v_File.size());
				}
				else {
					file.close();
				}
			}
			// if end
		}
		// for end
	}

	// get file count
	int FileCount = v_File.size();
	if (!FileCount) {
		std::cout << "No HCA found!\n";
		return;
	}

	v_header.sound_count = FileCount;
	v_AWEheader.sound_count = FileCount;
	v_AWEheader.fileTable_offset = (FileCount * 4) + 0x14;

	// in AWB
	int block_CueIDSize = FileCount * 4;
	int block_DataOfsSize = FileCount * 4;
	// in AWE
	int block_NameOfsSize = FileCount * 4;
	int block_IndexSize = FileCount * 2;

	// output file
	std::cout << "\nWriting AWB......\n";
	WriteAWBFile(inPath, block_CueIDSize, block_DataOfsSize);
	std::cout << "Writing AWE......\n";
	WriteAWEFile(inPath, block_NameOfsSize, block_IndexSize);
	std::cout << "Done!\n";
}

void AWB::WriteInitHeader()
{
	// AWB
	v_header.header = 844318273;
	v_header.pad4 = 1;
	v_header.dataOfs_size = 4;
	v_header.cueID_size = 4;
	v_header.sound_count = -2;
	v_header.align_size = 32;
	v_header.subkey = 0;

	// AWE
	v_AWEheader.header = 1161975617;
	v_AWEheader.version = 256;
	v_AWEheader.sound_count = -2;
	v_AWEheader.nameTable_offset = 0x14;
	v_AWEheader.fileTable_offset = -2;

	// note: -2 is used to check for errors
}

void AWB::WriteAWBFile(const std::string& inPath, int block_CueIDSize, int block_DataOfsSize)
{
	int HeaderSize = 0x10 + block_CueIDSize + block_DataOfsSize;
	int align = HeaderSize % 32;
	if (align) {
		HeaderSize += 32 - align;
	}

	std::vector<char> bytes(HeaderSize, 0);
	memcpy(&bytes[0], &v_header, 0x10);

	int cur_CueID = 0x10;
	int cur_DataOfs = 0x10 + block_CueIDSize;
	for (int i = 0; i < v_File.size(); i++) {
		WriteINT32LE(&bytes[cur_CueID], v_File[i].CueID);

		int dataOfs = HeaderSize + v_File[i].offset_data;
		WriteINT32LE(&bytes[cur_DataOfs], dataOfs);

		cur_CueID += 4;
		cur_DataOfs += 4;
	}

	std::ofstream newFile(inPath + ".awb", std::ios::binary | std::ios::out | std::ios::ate);
	newFile.write(bytes.data(), bytes.size());

	for (int i = 0; i < v_File.size(); i++) {
		newFile.write(v_File[i].data.data(), v_File[i].data.size());
	}

	newFile.close();
}

void AWB::WriteAWEFile(const std::string& inPath, int block_NameOfsSize, int block_IndexSize)
{
	int HeaderSize = 0x14 + block_NameOfsSize + block_IndexSize;

	std::vector<char> bytes(HeaderSize, 0);
	memcpy(&bytes[0], &v_AWEheader, 0x14);

	int cur_NameOfs = 0x14;
	int cur_Index = 0x14 + block_NameOfsSize;

	for (int i = 0; i < v_File.size(); i++) {

		int dataOfs = HeaderSize - cur_NameOfs + v_File[i].offset_name;
		WriteINT32LE(&bytes[cur_NameOfs], dataOfs);

		WriteUINT16LE(&bytes[cur_Index], v_File[i].index);

		cur_NameOfs += 4;
		cur_Index += 2;
	}

	std::ofstream newFile(inPath + "_list.awe", std::ios::binary | std::ios::out | std::ios::ate);
	newFile.write(bytes.data(), bytes.size());

	for (int i = 0; i < v_File.size(); i++) {
		newFile.write(v_File[i].name.data(), v_File[i].name.size()+1);
	}

	newFile.close();
}
