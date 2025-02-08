
#include <iostream>
#include <fstream>
#include <vector>
#include "d3dcompiler.h"

#pragma comment(lib, "d3dcompiler.lib")

std::string GetRaw(const std::vector<char>& buf)
{
	std::string str = "";
	unsigned char chunk;
	char tempbuffer[3];

	for (int i = 0; i < buf.size(); i++)
	{
		chunk = buf[i];
		if (chunk < 0x10)
			str += "0";

		_itoa_s(chunk, tempbuffer, 3, 16);
		str += tempbuffer;
	}
	return str;
}

void writeFile(std::wstring inFile, int filetype) {
	ID3DBlob* blob;
	ID3DBlob* error;
	HRESULT D3D = D3DCompileFromFile(inFile.c_str(),
		NULL, NULL,
		"main", "ps_5_0",
		D3DCOMPILE_OPTIMIZATION_LEVEL1, 0,
		&blob, &error);

	if (D3D == S_OK) {
		LPVOID shader_pointer = blob->GetBufferPointer();
		size_t shader_size = blob->GetBufferSize();

		std::vector< char > bytes(shader_size);
		memcpy(&bytes[0], shader_pointer, shader_size);


		std::wstring outPath;
		int pathSize = inFile.size();
		if (filetype == 1) {
			if (pathSize > 5) {
				outPath = inFile.substr(0, (pathSize - 5));
				outPath += L".txt";
			}
			else {
				outPath = inFile + L".txt";
			}

			std::string txt = GetRaw(bytes);

			std::ofstream newFile(outPath, std::ios::binary | std::ios::out | std::ios::ate);
			newFile.write(txt.c_str(), txt.size());
			newFile.close();
		}
		else {
			if (pathSize > 5) {
				outPath = inFile.substr(0, (pathSize - 5));
				outPath += L".dxbc";
			}
			else {
				outPath = inFile + L".dxbc";
			}

			std::ofstream newFile(outPath, std::ios::binary | std::ios::out | std::ios::ate);
			newFile.write(bytes.data(), bytes.size());
			/*
			for (int i = 0; i < bytes.size(); i++)
			{
				newFile << bytes[i];
			}*/
			newFile.close();
		}
		bytes.clear();

		std::wcout << L"Completion: " << outPath << L"\n";
	}
	else {
		LPVOID error_pointer = error->GetBufferPointer();
		size_t error_size = error->GetBufferSize();
		std::vector< char > bytes(error_size);
		memcpy(&bytes[0], error_pointer, error_size);
		std::cout << &bytes[0];
	}

	/*
	std::ifstream file(inFile, std::ios::binary | std::ios::ate | std::ios::in);

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	if (file.read(buffer.data(), size)) {
		ID3DBlob* blob;
		ID3DBlob* error;
		HRESULT D3D = D3DCompile((LPCVOID)buffer.data(), size,
								NULL, NULL, NULL,
								"ps_5_0", "ps_5_0",
								0, 0,
								&blob, &error);

		if (D3D == S_OK) {
			LPVOID shader_pointer = blob->GetBufferPointer();
			size_t shader_size = blob->GetBufferSize();

			std::vector< char > bytes(shader_size);
			memcpy(&bytes[0], shader_pointer, shader_size);

			std::ofstream newFile(inFile + L".hex", std::ios::binary | std::ios::out | std::ios::ate);
			for (int i = 0; i < bytes.size(); i++)
			{
				newFile << bytes[i];
			}
			newFile.close();

			bytes.clear();

			std::wcout << L"Completion: " << inFile << L".hex\n";
		}
		else {
			LPVOID error_pointer = error->GetBufferPointer();
			size_t error_size = error->GetBufferSize();
			std::vector< char > bytes(error_size);
			memcpy(&bytes[0], error_pointer, error_size);
			std::cout << &bytes[0];
		}
	}

	//Clear buffers
	buffer.clear();
	file.close();
	*/
}

void readFile(std::wstring inFile) {
	std::ifstream file(inFile, std::ios::binary | std::ios::ate | std::ios::in);

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	if (file.read(buffer.data(), size)) {
		ID3DBlob* blob;
		HRESULT D3D = D3DDisassemble((LPCVOID)&buffer[0], size, 0, "", &blob);

		if (D3D == S_OK) {
			LPVOID shader_pointer = blob->GetBufferPointer();
			size_t shader_size = blob->GetBufferSize();

			std::vector< char > bytes(shader_size);
			memcpy(&bytes[0], shader_pointer, shader_size);

			std::ofstream newFile(inFile + L".txt", std::ios::binary | std::ios::out | std::ios::ate);
			for (int i = 0; i < bytes.size(); i++)
			{
				newFile << bytes[i];
			}
			newFile.close();

			bytes.clear();

			std::wcout << L"Completion: " << inFile << L".txt\n";
		}
		else {
			std::wcout << L"Are you sure this is complete DXBC file?\n";
		}
	}

	//Clear buffers
	buffer.clear();
	file.close();
}

int wmain(int argc, wchar_t* argv[])
{
	std::wstring path;
	std::wstring type;

	if (argc > 1) {
		path = argv[1];
		//std::wcout << L"Parsing file: " << path << L"\n";
	}
	else {
		std::wcout << L"Filename: ";
		std::wcin >> path;
		std::wcout << L"\n";

		writeFile(path, 1);
		return 0;
		//std::wcout << L"Parsing......\n";
	}

	std::wcout << L"Type (0 is dxbc to asm, 1 is hlsl to binary, 2 is hlsl to txt): ";
	std::wcin >> type;
	std::wcout << L"\n";

	if (type == L"0") {
		readFile(path);
	}
	else if (type == L"1") {
		writeFile(path, 0);
	}
	else if (type == L"2") {
		writeFile(path, 1);
	}

	system("pause");
}
