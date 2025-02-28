#include "framework.h"
#include "ModelScale.h"

int __stdcall CheckModelXMLHeader(LPCSTR path, float scaleSize)
{
	tinyxml2::XMLDocument doc;
	doc.LoadFile(path);

	tinyxml2::XMLNode* header = doc.FirstChildElement("MDB");
	if (header) {
		int errorCode;
		errorCode = ScaleMDB(header, scaleSize);
		doc.SaveFile(path);
		return errorCode;
	}

	return -1;
}

int ScaleMDB(tinyxml2::XMLNode* header, float scaleSize)
{
	tinyxml2::XMLElement* entry, * entry2, * entry3;
	entry = header->FirstChildElement("BoneLists");
	if (entry)
	{
		float vf;
		for (entry2 = entry->FirstChildElement(); entry2 != 0; entry2 = entry2->NextSiblingElement("Bone"))
		{
			entry3 = entry2->FirstChildElement("mainTM");
			entry3 = entry3->NextSiblingElement();
			entry3 = entry3->NextSiblingElement();
			entry3 = entry3->NextSiblingElement();
			ScaleMDBFloat3(entry3, scaleSize);
			// skinTM
			entry3 = entry3->NextSiblingElement();
			entry3 = entry3->NextSiblingElement();
			entry3 = entry3->NextSiblingElement();
			entry3 = entry3->NextSiblingElement();
			ScaleMDBFloat3(entry3, scaleSize);
			// position
			entry3 = entry3->NextSiblingElement();
			ScaleMDBFloat3(entry3, scaleSize);
			// float
			entry3 = entry3->NextSiblingElement();
			ScaleMDBFloat3(entry3, scaleSize);
		}
	}
	else {
		return 1;
	}

	entry = header->FirstChildElement("ObjectLists");
	if (entry) {
		tinyxml2::XMLElement* pVertex, * pPos;
		for (entry2 = entry->FirstChildElement(); entry2 != 0; entry2 = entry2->NextSiblingElement("Object"))
		{
			//--------------------------------------------
			for (entry3 = entry2->FirstChildElement("Mesh"); entry3 != 0; entry3 = entry3->NextSiblingElement("Mesh"))
			{
				pVertex = entry3->FirstChildElement("VertexList")->FirstChildElement("position");
				if (pVertex) {
					for (pPos = pVertex->FirstChildElement("V"); pPos != 0; pPos = pPos->NextSiblingElement("V")) {
						ScaleMDBFloat3(pPos, scaleSize);
					}
				}
			}
			//--------------------------------------------
		}
	}
	else {
		return 2;
	}
	// end
	return 0;
}

void __fastcall ScaleMDBFloat3(tinyxml2::XMLElement* data, float scaleSize)
{
	float vf;
	vf = data->FloatAttribute("x");
	vf *= scaleSize;
	data->SetAttribute("x", vf);

	vf = data->FloatAttribute("y");
	vf *= scaleSize;
	data->SetAttribute("y", vf);

	vf = data->FloatAttribute("z");
	vf *= scaleSize;
	data->SetAttribute("z", vf);
}
