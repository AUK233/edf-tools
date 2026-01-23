#pragma once
#include "include/tinyxml2.h"

enum class DataType_t
{
	None,
	FromXML,
	ToSGO,
	FromSGO,
	ToMAB,
	FromMAB,
	ToMTAB,
	FromMTAB,
	ToCANM,
	FromCANM,
	ToCAS,
	FromCAS,
	ToBVM,
	FromBVM,
	ToRAB,
	FromRAB,
	ToMBD,
	FromMBD,
	ToRMPA6,
	FromRMPA6,
};

enum class ProcessType_t
{
	None,
	Batch,
	Pack,
	BatchToPackage
};

enum class ThreadType_t
{
	None,
	MultiThreading,
	MultiCore,
	SetThread,
	NoCompression,
	// to use batch
	Read,
	Write
};

// Check for the extra file header
void CheckDataType(const std::vector<char>& buffer, tinyxml2::XMLElement*& xmlHeader, const std::string& str);
// write
// Check the header to determine the output type
void CheckXMLHeader(const std::wstring& path);
// Check for the extra file header when writing
std::vector< char > CheckDataType(tinyxml2::XMLElement* Data, tinyxml2::XMLNode* header);

// Now processing files here
void ProcessFile(const std::wstring& path, ProcessType_t processType, ThreadType_t threadType, int threadNum);
void ProcessFile_Pack(const std::wstring& path, ThreadType_t threadType, int threadNum);
// 0 is not directory
int ProcessFile_Batch(const std::wstring& inPath, const std::wstring& inExtension, ThreadType_t threadType);
void ProcessFile_BatchToPackage(const std::wstring& path, ThreadType_t threadType, int threadNum);
DataType_t ProcessFile_CheckType_Write(const std::wstring& inPath, const std::wstring& inExtension, int bvmFlags);
DataType_t ProcessFile_CheckType_Read(const std::wstring& inPath, const std::wstring& inExtension);
