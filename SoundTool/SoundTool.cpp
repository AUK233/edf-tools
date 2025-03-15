#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>

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

	unique_ptr< AWB > script = make_unique< AWB >();
	script->Read(path.substr(0, path.size()-4) );
	script.reset();

	system("pause");
	return 0;
}
