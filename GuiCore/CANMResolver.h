#pragma once
#include "..\EDF_Tools\include\tinyxml2.h"

extern "C" _declspec(dllexport) int __stdcall CheckCanmXMLVersion(LPCSTR inPath, LPCSTR outPath);
int CheckCANMVersion(tinyxml2::XMLElement* inData, LPCSTR outPath);

// 6 to 5
int Canm6To5(tinyxml2::XMLElement* inData, LPCSTR outPath);
void Canm6To5Keyframe(tinyxml2::XMLElement* inData, tinyxml2::XMLElement* outData);
void Canm6To5SetTransform(tinyxml2::XMLElement* inData, tinyxml2::XMLElement* outData);
void Canm6To5SetQuaternionToEuler(tinyxml2::XMLElement* inData, tinyxml2::XMLElement* outData);

void Canm6To5SetFloatToUInt16(tinyxml2::XMLElement* inData, tinyxml2::XMLElement* outData);
// 6 to 5 end

void CanmResolverGetVector3(tinyxml2::XMLElement* in, UINT16* vf);
void CanmResolverGetVector3(tinyxml2::XMLElement* in, float* vf);
void CanmResolverGetVector3(tinyxml2::XMLElement* in, double* vf);

void CanmResolverSetVector3(tinyxml2::XMLElement* out, const UINT16* vf);

void CanmResolverGetVector4(tinyxml2::XMLElement* in, double* vf);
void CanmResolverQuaternionToEuler(const double* in, double* out);
