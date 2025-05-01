#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>
#include <algorithm>

#include "clACB.h"
#include "clAWB.h"

int main(int argc, char* argv[])
{
	using namespace std;

	string path;
	if (argc > 1) {
		path = argv[1];
	}
	else {
		cout << "Filename: ";
		cin >> path;
		cout << "\n";
	}

	size_t lastDotPos = path.find_last_of('.');
	if (lastDotPos != string::npos) {
		string extension = path.substr(lastDotPos + 1, path.size() - lastDotPos);
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

		// AWB and ACB format information from https://github.com/vgmstream/vgmstream
		if (extension == "awb" || extension == "awe") {
			unique_ptr< AWB > script = make_unique< AWB >();
			script->Read(path.substr(0, path.size() - 4));
			script.reset();
		}
		else if (extension == "acb") {
			unique_ptr< ACB > script = make_unique< ACB >();
			script->Read(path.substr(0, path.size() - 4));
			script.reset();
		}
		else {
			cout << "Please input an AWB/AWE or ACB file\n";
		}
		// end
	}
	else {
		// write AWB and AWE
		unique_ptr< AWB > script = make_unique< AWB >();
		script->Write(path);
		script.reset();
	}


	system("pause");
	return 0;
}
