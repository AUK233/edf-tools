#include <iostream>
#include <vector>

#include "..\EDF_Tools\include\tinyxml2.h"

void remapBoneName(const std::string& source, const std::string& remapSource) {
	tinyxml2::XMLDocument doc, remapDoc;
	doc.LoadFile(source.c_str());
	remapDoc.LoadFile(remapSource.c_str());

	tinyxml2::XMLNode* header = doc.FirstChildElement("MDB");
	tinyxml2::XMLNode* remapheader = remapDoc.FirstChildElement("MDB");

	// load bone information
	tinyxml2::XMLElement* boneXML = header->FirstChildElement("BoneLists");
	tinyxml2::XMLElement* remapBoneXML = remapheader->FirstChildElement("BoneLists");
	if (boneXML == nullptr) {
		std::cout << "Missing source file bone list.\n";
		return;
	}
	if (remapheader == nullptr) {
		std::cout << "Missing remapping file bone list.\n";
		return;
	}
	// load mapping bone list
	tinyxml2::XMLElement* pBone, *pBoneName;
	std::vector<std::string> v_remapList;
	for (pBone = remapBoneXML->FirstChildElement("Bone"); pBone != 0; pBone = pBone->NextSiblingElement("Bone"))
	{
		pBoneName = pBone->FirstChildElement("name");
		v_remapList.push_back(pBoneName->GetText());
	}
	// remapping bone list
	std::vector<int> v_BoneList;
	std::vector<std::string> v_MissList;
	int remapVSize = v_remapList.size();
	for (pBone = boneXML->FirstChildElement("Bone"); pBone != 0; pBone = pBone->NextSiblingElement("Bone"))
	{
		std::string bonename = pBone->FirstChildElement("name")->GetText();
		int newIndex = 0;
		int isMiss = 1;
		for (int i = 0; i < remapVSize; i++) {
			newIndex = i;
			if (bonename == v_remapList[i]) {
				isMiss = 0;
				break;
			}
		}
		v_BoneList.push_back(newIndex);

		if (isMiss) {
			v_MissList.push_back(bonename);
		}
	}
	// check the missing list
	int missVSize = v_MissList.size();
	if (missVSize) {
		std::cout << "Missing the following bones:\n";
		for (int i = 0; i < missVSize; i++) {
			std::cout << v_MissList[i];
			std::cout << "\n";
		}

		int type = 0;
		std::wcout << L"\nContinue? (0 is no, 1 is yes!): ";
		std::wcin >> type;
		if (!type) {
			return;
		}
	}

	// load model mesh
	tinyxml2::XMLElement* pObjectList = header->FirstChildElement("ObjectLists");
	if (pObjectList == nullptr) {
		std::cout << "\n\nMissing source file ObjectLists.\n";
		return;
	}
	// update skin information
	tinyxml2::XMLElement *pObject, *pMesh, *pVertex, *pSkin;
	int skinIndex;
	for (pObject = pObjectList->FirstChildElement("Object"); pObject != 0; pObject = pObject->NextSiblingElement("Object"))
	{
		// 1
		for (pMesh = pObject->FirstChildElement("Mesh"); pMesh != 0; pMesh = pMesh->NextSiblingElement("Mesh"))
		{
			// 2
			pVertex = pMesh->FirstChildElement("VertexList")->FirstChildElement("BLENDINDICES");
			if (pVertex) {
				for (pSkin = pVertex->FirstChildElement("V"); pSkin != 0; pSkin = pSkin->NextSiblingElement("V")) {
					skinIndex = pSkin->IntAttribute("x");
					if (skinIndex) {
						pSkin->SetAttribute("x", v_BoneList[skinIndex]);
					}

					skinIndex = pSkin->IntAttribute("y");
					if (skinIndex) {
						pSkin->SetAttribute("y", v_BoneList[skinIndex]);
					}

					skinIndex = pSkin->IntAttribute("z");
					if (skinIndex) {
						pSkin->SetAttribute("z", v_BoneList[skinIndex]);
					}

					skinIndex = pSkin->IntAttribute("w");
					if (skinIndex) {
						pSkin->SetAttribute("w", v_BoneList[skinIndex]);
					}
				}
			}
			// 2 end
		}
		// 1 end
	}

	std::string outPath = source.substr(0, (source.size() - 4));
	outPath += "_MDB.xml";
	doc.SaveFile(outPath.c_str());
	std::cout << "\nDONE!\n";
}

int main(int argc, char* argv[])
{
	using namespace std;

	string sourcePath, remapSource;

	//sourcePath = "Z:\\TEMP\\source.xml";
	//remapSource = "Z:\\TEMP\\remap.xml";

	if (argc > 1) {
		sourcePath = argv[1];
	}
	else {
		cout << "Source file for bones need to be remapped:\n";
		cin >> sourcePath;
		cout << "\n";
	}

	if (argc > 2) {
		remapSource = argv[2];
	}
	else {
		cout << "File as mapped bones:\n";
		cin >> remapSource;
		cout << "\n";
	}

	remapBoneName(sourcePath, remapSource);

	system("pause");
}
