#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>

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
	ReadDataOffset(inPath, buffer);

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

void AWB::ReadDataOffset(const std::string& inPath, const std::vector<char>& buffer)
{
	int base_ofs = 0x10 + (v_header.sound_count * v_header.cueID_size);

	int newSoundCount = v_header.sound_count - 1;
	for (int i = 0; i <= newSoundCount; i++) {
		int CueID_offset = 0x10 + (i * v_header.cueID_size);

		int CueID = ReadUINT16LE(&buffer[CueID_offset]);

		int cur_ofs = base_ofs + (i * v_header.dataOfs_size);

		FileOffset_t data;
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
		/*
		int subfile_size = data.next - data.self; 
		if (subfile_size) {
			std::vector<char> subfile_buffer(buffer.begin() + data.self, buffer.begin() + data.next);

			int FolderExists = DirectoryExists(inPath.c_str());
			if (!FolderExists) {
				CreateDirectoryA(inPath.c_str(), NULL);
			}

			std::ofstream newFile(inPath + "\\" + std::to_string(data.self) + ".bin", std::ios::binary | std::ios::out | std::ios::ate);
			newFile.write(subfile_buffer.data(), subfile_buffer.size());
			newFile.close();
			
		}*/
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
}

void AWB::ReadDataToFile(const std::string& inPath, const std::vector<char>& buffer)
{
	for (int i = 0; i < v_NameTable.size(); i++) {

		int FolderExists = DirectoryExists(inPath.c_str());
		if (!FolderExists) {
			CreateDirectoryA(inPath.c_str(), NULL);
		}

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
