#pragma once
#include "..\EDF_Tools\include\tinyxml2.h"

extern "C" _declspec(dllexport) int __stdcall CheckModelXMLHeader(LPCSTR path, float scaleSize);
int ScaleMDB(tinyxml2::XMLNode* header, float scaleSize);
void __fastcall ScaleMDBFloat3(tinyxml2::XMLElement* data, float scaleSize);
int ScaleCANM(tinyxml2::XMLElement* data, float scaleSize);
void __fastcall ScaleCANMFloat3x2(tinyxml2::XMLElement* data, float scaleSize);
