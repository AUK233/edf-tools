
#include <iostream>
#include <fstream>
#include <vector>
#include "d3dcompiler.h"

#pragma comment(lib, "d3dcompiler.lib")

void writeFile(std::wstring inFile) {
	ID3DBlob* blob;
	ID3DBlob* error;
	HRESULT D3D = D3DCompileFromFile(inFile.c_str(),
		NULL, NULL,
		"main", "ps_5_0",
		D3DCOMPILE_SKIP_OPTIMIZATION, 0,
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

	if (argc > 1) {
		path = argv[1];
		//std::wcout << L"Parsing file: " << path << L"\n";
	}
	else {
		std::wcout << L"Filename:";
		std::wcin >> path;
		std::wcout << L"\n";

		//std::wcout << L"Parsing......\n";
	}

	std::wstring type;
	std::wcout << L"Type:";
	std::wcin >> type;
	std::wcout << L"\n";

	if (type == L"0") {
		readFile(path);
	}
	else if (type == L"1") {
		writeFile(path);
	}

	system("pause");
}
