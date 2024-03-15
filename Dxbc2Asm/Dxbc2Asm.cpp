
#include <iostream>
#include <fstream>
#include <vector>
#include "d3dcompiler.h"

#pragma comment(lib, "d3dcompiler.lib")

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
		std::wcout << L"Parsing file: " << path << L"\n";
	}
	else {
		std::wcout << L"Filename:";
		std::wcin >> path;
		std::wcout << L"\n";

		std::wcout << L"Parsing......\n";
	}

	readFile(path);

	system("pause");
}
