#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>

#include "util.h"

#include "SGO.h" //SGO parser
#include "DSGO.h" //DSGO parser
#include "MAB.h" //MAB parser
#include "MTAB.h" //MTAB parser

#include "MDB.h" //MDB parser
#include "CAS.h" //CAS parser
#include "CANM.h" //CANM parser
#include "RMPA6.h" //EDF6's RMPA
#include "RAB.h" //RAB extractor

#include "JSONAMLParser.h"
#include "MissionScript.h" //TODO: Implement mission script class that stores and proccess data

#include "Middleware.h"

void CheckDataType(const std::vector<char>& buffer, tinyxml2::XMLElement*& xmlHeader, const std::string& str)
{
	char header[4];
	header[0] = buffer[0];
	header[1] = buffer[1];
	header[2] = buffer[2];
	header[3] = buffer[3];

	char mtabheader[4];
	mtabheader[0] = buffer[0x10];
	mtabheader[1] = buffer[0x11];
	mtabheader[2] = buffer[0x12];
	mtabheader[3] = buffer[0x13];

	tinyxml2::XMLElement* NewXml = xmlHeader->InsertNewChildElement("Subdata");
	NewXml->SetAttribute("name", str.c_str());

	if ((header[0] == 0x53 && header[1] == 0x47 && header[2] == 0x4f && header[3] == 0x00) || (header[3] == 0x53 && header[2] == 0x47 && header[1] == 0x4f && header[0] == 0x00))
	{
		NewXml->SetAttribute("header", "SGO");

		std::unique_ptr< SGO > sgoReader = std::make_unique< SGO >();
		sgoReader->ReadData(buffer, NewXml, xmlHeader);
		sgoReader.reset();
	}
	else if (header[0] == 0x4D && header[1] == 0x41 && header[2] == 0x42 && header[3] == 0x00)
	{
		NewXml->SetAttribute("header", "MAB");

		std::unique_ptr< MAB > mabReader = std::make_unique< MAB >();
		mabReader->ReadData(buffer, NewXml, xmlHeader);
		mabReader.reset();
	}
	else if (mtabheader[0] == 0x4D && mtabheader[1] == 0x54 && mtabheader[2] == 0x41 && mtabheader[3] == 0x42 && header[2] == 0x00 && header[3] == 0x00)
	{
		NewXml->SetAttribute("header", "MTAB");

		std::unique_ptr< MTAB > mtabReader = std::make_unique< MTAB >();
		mtabReader->ReadData(buffer, NewXml, xmlHeader);
		mtabReader.reset();
	}
	else
	{
		NewXml->SetAttribute("header", "RAW");

		std::string rawstr = ReadRaw(buffer, 0, buffer.size());
		NewXml->SetText(rawstr.c_str());
	}
}

// Check the header to determine the output type
void CheckXMLHeader(const std::wstring& path)
{
	std::wstring sourcePath = path + L"_data.xml";
	std::string UTF8Path = WideToUTF8(sourcePath);

	tinyxml2::XMLDocument doc;
	doc.LoadFile(UTF8Path.c_str());

	tinyxml2::XMLNode* header = doc.FirstChildElement("EDFDATA");
	tinyxml2::XMLElement* mainData = header->FirstChildElement("Main");

	std::string headerType = mainData->Attribute("header");
	if (headerType == "SGO")
	{
		std::unique_ptr< SGO > writer = std::make_unique< SGO >();
		writer->Write(path, header);
		writer.reset();
	}
	else if (headerType == "MAB")
	{
		std::unique_ptr< MAB > writer = std::make_unique< MAB >();
		writer->Write(path, header);
		writer.reset();
	}
	else if (headerType == "MTAB")
	{
		std::unique_ptr< MTAB > writer = std::make_unique< MTAB >();
		writer->Write(path, header);
		writer.reset();
	}
	else if (headerType == "DSGO")
	{
		std::unique_ptr< DSGO > writer = std::make_unique< DSGO >();
		writer->Write(path, header);
		writer.reset();
	}
}

// Check for the extra file header
std::vector<char> CheckDataType(tinyxml2::XMLElement* Data, tinyxml2::XMLNode* header)
{
	std::vector<char> bytes;

	std::string headerType = Data->Attribute("header");
	if (headerType == "SGO")
	{
		std::unique_ptr< SGO > writer = std::make_unique< SGO >();
		bytes = writer->WriteData(Data, header);
		writer.reset();
	}
	else if (headerType == "MAB")
	{
		std::unique_ptr< MAB > writer = std::make_unique< MAB >();
		bytes = writer->WriteData(Data, header);
		writer.reset();
	}
	else if (headerType == "MTAB")
	{
		std::unique_ptr< MTAB > writer = std::make_unique< MTAB >();
		bytes = writer->WriteData(Data, header);
		writer.reset();
	}
	else if (headerType == "DSGO")
	{
		std::unique_ptr< DSGO > writer = std::make_unique< DSGO >();
		bytes = writer->WriteData(Data, header);
		writer.reset();
	}
	else if (headerType == "RAW")
	{
		std::string argsStrn = Data->GetText();
		if (argsStrn.length() % 2 > 0)
		{
			argsStrn = ("0" + argsStrn);
		}
		int count = 0;
		//Convert to hex.
		char byte;
		for (unsigned int i = 0; i < argsStrn.length(); i += 2)
		{
			std::string byteString = argsStrn.substr(i, 2);
			byte = (char)std::stol(byteString.c_str(), NULL, 16);
			bytes.push_back(byte);
			count++;
		}
	}

	//std::wcout << L"\ndata size: " + ToString(int(bytes.size())) + L"\n\n";

	return bytes;
}

void ProcessFile(const std::wstring& path, ProcessType_t processType, ThreadType_t threadType, int threadNum)
{
	if (std::filesystem::is_directory(path)) {
		switch (processType)
		{
		case ProcessType_t::Batch:
			ProcessFile_Batch(path, L"", threadType);
			return;
		case ProcessType_t::Pack:
			ProcessFile_Pack(path, threadType, threadNum);
			return;
		case ProcessType_t::BatchToPackage:
			ProcessFile_BatchToPackage(path, threadType, threadNum);
			return;
		default:
			ProcessFile_Pack(path, ThreadType_t::NoCompression, 0);
			return;
		}
	}

	//Get file extension:
	size_t lastindex = path.find_last_of(L'.');

	if (lastindex != std::wstring::npos){
		std::wstring extension = path.substr(lastindex, path.size() - lastindex);
		std::wstring strn = path.substr(0, lastindex);
		//List all files
		if (strn == L"*") {
			ProcessFile_Batch(std::filesystem::current_path().wstring(), extension, ThreadType_t::Write);
		} else {
			auto result = ProcessFile_CheckType_Write(strn, extension, 1);
			if (result == DataType_t::None) {
				ProcessFile_CheckType_Read(strn, extension);
			}
		}
		// end
	}

	system("pause");
}

void ProcessFile_Pack(const std::wstring& path, ThreadType_t threadType, int threadNum)
{
	std::wcout << L"Now, pack the folder.\n";

	std::unique_ptr< RAB > rabReader = std::make_unique< RAB >();
	rabReader->Initialization();

	switch (threadType)
	{
		case ThreadType_t::NoCompression: {
			rabReader->bUseFakeCompression = true;
			break;
		}
		case ThreadType_t::MultiThreading: {
			rabReader->bIsMultipleThreads = true;
			break;
		}
		case ThreadType_t::MultiCore: {
			// Initialize thread information
			rabReader->WriteInitMTInfo();
			break;
		}
		case ThreadType_t::SetThread: {
			rabReader->bIsMultipleThreads = true;
			rabReader->bIsMultipleCores = true;
			rabReader->customizeThreads = 4;
			if (threadNum > 0) {
				rabReader->customizeThreads = threadNum;
			}
			break;
		}
		default: {
			break;
		}
	}

	std::wstring fileName = path;
	rabReader->CreateFromDirectory(fileName);

	if (rabReader->esbFileNum) {
		fileName += L".efarc";
		goto writeRAB;
	}

	if (rabReader->mdbFileNum > 1)
	{
		fileName += L".mrab";
	} else {
		fileName += L".rab";
	}

writeRAB:
	rabReader->Write(fileName);
	rabReader.reset();
}

#define FLAG_VERBOSE 1
#define FLAG_CREATE_FOLDER 2

int ProcessFile_Batch(const std::wstring& inPath, const std::wstring& inExtension, ThreadType_t threadType)
{
	//std::wcout << inPath << L"\n";
	//std::wcout << inExtension << L"\n";

	if (!std::filesystem::is_directory(inPath)) return 0;

	struct vFILE_t {
		std::wstring path;
		std::wstring extension;
	};

	std::vector<vFILE_t> v_file;
	for (const auto& entry : std::filesystem::directory_iterator(inPath)) {
		if (entry.is_regular_file()) {
			auto extension = entry.path().extension().wstring();
			if (!inExtension.empty()) {
				if (extension != inExtension) continue;
			}

			vFILE_t out;
			out.extension = extension;
			auto filepath = entry.path().wstring();
			size_t lastindex = filepath.find_last_of(L'.');
			if (lastindex != std::wstring::npos) {
				out.path = filepath.substr(0, lastindex);
				v_file.push_back(out);
			}
		}
		// for end
	}

	for (int i = 0; i < v_file.size(); i++) {
		//std::wcout << v_file[i].path << L"\n";
		if (threadType == ThreadType_t::Read) {
			ProcessFile_CheckType_Read(v_file[i].path, v_file[i].extension);
		}
		else if (threadType == ThreadType_t::Write) {
			ProcessFile_CheckType_Write(v_file[i].path, v_file[i].extension, 1 | FLAG_CREATE_FOLDER);
		}
	}

	return 1;
}

void ProcessFile_BatchToPackage(const std::wstring& path, ThreadType_t threadType, int threadNum)
{
	if (!std::filesystem::is_directory(path)) return;

	auto modelPath = path + L"\\MODEL";
	ProcessFile_Batch(modelPath, L".xml", ThreadType_t::Write);

	ProcessFile_Pack(path, threadType, threadNum);
}

DataType_t ProcessFile_CheckType_Write(const std::wstring& inPath, const std::wstring& inExtension, int bvmFlags)
{
	auto extension = ConvertToLower(inExtension);
	if (extension == L".txt")
	{
		std::unique_ptr<CMissionScript> script = std::make_unique<CMissionScript>();
		script->Write(inPath, bvmFlags);
		script.reset();
		return DataType_t::ToBVM;
	}

	if (extension == L".xml")
	{
		size_t xmlIndex = inPath.find_last_of(L'_');
		std::wstring xmlExtension = inPath.substr(xmlIndex + 1, inPath.size() - xmlIndex);
		std::wstring xmlStrn = inPath.substr(0, xmlIndex);

		xmlExtension = ConvertToLower(xmlExtension);

		if (xmlExtension == L"mdb") {
			// To MDB File, no need for multi-core.
			std::unique_ptr< CXMLToMDB > script = std::make_unique< CXMLToMDB >();
			script->Write(xmlStrn, false);
			script.reset();
			return DataType_t::FromXML;
		}
		
		if (xmlExtension == L"cas") {
			std::unique_ptr< CAS > script = std::make_unique< CAS >();
			script->Write(xmlStrn);
			script.reset();
			return DataType_t::FromXML;
		}
		
		if (xmlExtension == L"canm") {
			std::unique_ptr< CANM > script = std::make_unique< CANM >();
			script->Write(xmlStrn);
			script.reset();
			return DataType_t::FromXML;
		}
		
		if (xmlExtension == L"data") {
			//Data needs a function to judge the header.
			CheckXMLHeader(xmlStrn);
			return DataType_t::FromXML;
		}
		
		if (xmlExtension == L"rmpa") {
			std::unique_ptr< RMPA6 > script = std::make_unique< RMPA6 >();
			script->Write(xmlStrn);
			script.reset();
			return DataType_t::FromXML;
		}
	}

	// end
	return DataType_t::None;
}

DataType_t ProcessFile_CheckType_Read(const std::wstring& inPath, const std::wstring& inExtension)
{
	auto extension = ConvertToLower(inExtension);

	if (extension == L".bvm") {
		std::unique_ptr<CMissionScript> script = std::make_unique<CMissionScript>();
		script->LoadLanguage(L"EDF5_2C_MissionCommands.jsonaml", 0);
		script->LoadLanguage(L"EDF5_2D_MissionCommands.jsonaml", 1);
		script->Read(inPath);
		script.reset();
		return DataType_t::FromBVM;
	}

	if (extension == L".rmpa") {
		std::unique_ptr<RMPA6> rmpa = std::make_unique<RMPA6>();
		rmpa->Read(inPath);
		rmpa.reset();
		return DataType_t::FromRMPA6;
	}

	if (extension == L".rab") {
		std::unique_ptr< RAB > rabReader = std::make_unique< RAB >();
		rabReader->Read(inPath, L".rab");
		rabReader.reset();
		return DataType_t::FromRAB;
	}
	
	if (extension == L".mrab") {
		std::unique_ptr< RAB > rabReader = std::make_unique< RAB >();
		rabReader->Read(inPath, L".mrab");
		rabReader.reset();
		return DataType_t::FromRAB;
	}
	
	if (extension == L".efarc") {
		// Has a fatal bug, don't read efarc in non-RAMDisk.
		std::unique_ptr< RAB > rabReader = std::make_unique< RAB >();
		rabReader->Read(inPath, L".efarc");
		rabReader.reset();
		return DataType_t::FromRAB;
	}
	
	if (extension == L".mdb") {
		/*
		wstring scstr;
		wcout << L"Single Core? (0 is false, 1 is true) : ";
		wcin >> scstr;
		bool onecore = false;
		if (stoi(scstr) == 1)
		{
			onecore = true;
			wcout << L"\nWill now use a single core to read the file!";
		}
		wcout << L"\n\n";
		*/

		std::unique_ptr<CMDBtoXML> script = std::make_unique<CMDBtoXML>();
		script->Read(inPath, true);
		script.reset();
		return DataType_t::FromBVM;
	}

	if (extension == L".sgo") {
		std::unique_ptr< SGO > sgoReader = std::make_unique< SGO >();
		sgoReader->Read(inPath);
		sgoReader.reset();
		return DataType_t::FromSGO;
	}
	
	if (extension == L".mab") {
		std::unique_ptr< MAB > mabReader = std::make_unique< MAB >();
		mabReader->Read(inPath);
		mabReader.reset();
		return DataType_t::FromMAB;
	}
	
	if (extension == L".mtab") {
		std::unique_ptr< MTAB > mtabReader = std::make_unique< MTAB >();
		mtabReader->Read(inPath);
		mtabReader.reset();
		return DataType_t::FromMTAB;
	}
	
	if (extension == L".cas") {
		std::unique_ptr<CAS> casReader = std::make_unique<CAS>();
		casReader->Read(inPath);
		casReader.reset();
		return DataType_t::FromCAS;
	}
	
	if (extension == L".canm") {
		std::unique_ptr<CANM> canmReader = std::make_unique<CANM>();
		canmReader->Read(inPath);
		canmReader.reset();
		return DataType_t::FromCANM;
	}

	return DataType_t::None;
}
