#include "framework.h"
#include "ModelScale.h"

int __stdcall CheckModelXMLHeader(LPCSTR path, float scaleSize)
{
	tinyxml2::XMLDocument doc;
	doc.LoadFile(path);

	tinyxml2::XMLElement* header = doc.FirstChildElement();
	if (header) {
		std::string nodeType = header->Name();

		if (nodeType == "MDB")
		{
			int errorCode;
			errorCode = ScaleMDB(header, scaleSize);
			doc.SaveFile(path);
			return errorCode;
		}
		else if(nodeType == "CAS"){
			tinyxml2::XMLElement* CanmData = header->FirstChildElement("CanmData");

			int errorCode;
			errorCode = ScaleCANM(CanmData, scaleSize);
			doc.SaveFile(path);
			return errorCode;
		}
		else if (nodeType == "CANM") {
			int errorCode;
			errorCode = ScaleCANM(header, scaleSize);
			doc.SaveFile(path);
			return errorCode;
		}
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

int ScaleCANM(tinyxml2::XMLElement* data, float scaleSize)
{
	tinyxml2::XMLElement* entry, * entry2, * entry3, * entry4;
	std::string type;
	entry = data->FirstChildElement("AnmData");
	for (entry2 = entry->FirstChildElement("node"); entry2 != 0; entry2 = entry2->NextSiblingElement("node"))
	{
		for (entry3 = entry2->FirstChildElement("value"); entry3 != 0; entry3 = entry3->NextSiblingElement("value"))
		{
			entry4 = entry3->FirstChildElement("position");
			type = entry4->Attribute("type");
			if (type != "null")
			{
				ScaleCANMFloat3x2(entry4, scaleSize);
			}
		}
	}
	return 0;
}

void __fastcall ScaleCANMFloat3x2(tinyxml2::XMLElement* data, float scaleSize)
{
	float vf;
	vf = data->FloatAttribute("ix");
	vf *= scaleSize;
	data->SetAttribute("ix", vf);

	vf = data->FloatAttribute("iy");
	vf *= scaleSize;
	data->SetAttribute("iy", vf);

	vf = data->FloatAttribute("iz");
	vf *= scaleSize;
	data->SetAttribute("iz", vf);
	//
	vf = data->FloatAttribute("vx");
	vf *= scaleSize;
	data->SetAttribute("vx", vf);

	vf = data->FloatAttribute("vy");
	vf *= scaleSize;
	data->SetAttribute("vy", vf);

	vf = data->FloatAttribute("vz");
	vf *= scaleSize;
	data->SetAttribute("vz", vf);
}


