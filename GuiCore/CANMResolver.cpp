#include "framework.h"
#include <immintrin.h>
#include <vector>
#define M_PI 3.14159265358979323846 // pi
#include <cmath>

#include "CANMResolver.h"

int __stdcall CheckCanmXMLVersion(LPCSTR inPath, LPCSTR outPath)
{
	tinyxml2::XMLDocument inDoc;
	inDoc.LoadFile(inPath);

	tinyxml2::XMLElement* inHeader = inDoc.FirstChildElement();

	if (inHeader) {
		std::string nodeType = inHeader->Name();

		if (nodeType == "CAS") {
			tinyxml2::XMLElement* CanmData = inHeader->FirstChildElement("CanmData");

			int errorCode = CheckCANMVersion(CanmData, outPath);
			return errorCode;
		}
		else if (nodeType == "CANM") {
			int errorCode = CheckCANMVersion(inHeader, outPath);
			return errorCode;
		}
	}

	//doc.SaveFile(outPath);

	return -1;
}

int CheckCANMVersion(tinyxml2::XMLElement* inData, LPCSTR outPath)
{
	int errorCode = -1;
	int version = inData->IntAttribute("version", 0);

	if (version == 6) {
		errorCode = Canm6To5(inData, outPath);
	}

	return errorCode;
}

int Canm6To5(tinyxml2::XMLElement* inData, LPCSTR outPath)
{
	int errorCode = -1;

	tinyxml2::XMLDocument xml;
	xml.InsertFirstChild(xml.NewDeclaration());
	tinyxml2::XMLElement* xmlHeader = xml.NewElement("CANM");
	xml.InsertEndChild(xmlHeader);
	//xml.SaveFile(outPath);

	tinyxml2::XMLElement* xmlData = xmlHeader->InsertNewChildElement("AnmData");
	tinyxml2::XMLElement* xmlPTR;
	tinyxml2::XMLElement* xmlNode;
	tinyxml2::XMLElement* xmlKF;

	// read data
	tinyxml2::XMLElement* entry, * entry2, * entry3, * entry4;
	LPCSTR pStr;
	int i32_value;
	float f32_value;

	entry = inData->FirstChildElement("AnmData");
	for (entry2 = entry->FirstChildElement("node"); entry2 != 0; entry2 = entry2->NextSiblingElement("node"))
	{
		// create out node
		xmlPTR = xmlData->InsertNewChildElement("node");

		// set node value
		i32_value = entry2->IntAttribute("index");
		xmlPTR->SetAttribute("index", i32_value);
		
		i32_value = entry2->IntAttribute("loop");
		xmlPTR->SetAttribute("int1", i32_value);

		pStr = entry2->Attribute("name");
		xmlPTR->SetAttribute("name", pStr);

		f32_value = entry2->FloatAttribute("time");
		xmlPTR->SetAttribute("time", f32_value);

		f32_value = entry2->FloatAttribute("speed");
		xmlPTR->SetAttribute("speed", f32_value);

		i32_value = entry2->IntAttribute("keyframe");
		xmlPTR->SetAttribute("kf", i32_value);
		// node value end

		for (entry3 = entry2->FirstChildElement("value"); entry3 != 0; entry3 = entry3->NextSiblingElement("value"))
		{
			// create out value
			xmlNode = xmlPTR->InsertNewChildElement("value");

			pStr = entry3->Attribute("bone");
			xmlNode->SetAttribute("bone", pStr);

			entry4 = entry3->FirstChildElement("position");
			xmlKF = xmlNode->InsertNewChildElement("position");
			Canm6To5Keyframe(entry4, xmlKF);

			entry4 = entry3->FirstChildElement("rotation");
			xmlKF = xmlNode->InsertNewChildElement("rotation");
			Canm6To5Keyframe(entry4, xmlKF);

			entry4 = entry3->FirstChildElement("scaling");
			xmlKF = xmlNode->InsertNewChildElement("scaling");
			Canm6To5Keyframe(entry4, xmlKF);

		}
	}

	xml.SaveFile(outPath);
	errorCode = 0;

	return errorCode;
}

void Canm6To5Keyframe(tinyxml2::XMLElement* inData, tinyxml2::XMLElement* outData)
{
	int i_type;
	tinyxml2::XMLElement* outPtr;

	std::string type = inData->Attribute("type");
	if (type == "null")
	{
		outData->SetAttribute("type", "null");
	}
	else {
		i_type = inData->IntAttribute("type");
		switch (i_type)
		{
		case 0: {
			outData->SetAttribute("type", "0");
			Canm6To5SetTransform(inData, outData);
			break;
		}
		case 1: {
			outData->SetAttribute("type", "1");
			Canm6To5SetTransform(inData, outData);
			UINT16 i16_value[3];
			for (tinyxml2::XMLElement* inKF = inData->FirstChildElement("keyframe")->FirstChildElement("v"); inKF != 0; inKF = inKF->NextSiblingElement("v")) {

				CanmResolverGetVector3(inKF, i16_value);
				outPtr = outData->InsertNewChildElement("v");
				CanmResolverSetVector3(outPtr, i16_value);

			}
			break;
		}
		case 2: {
			outData->SetAttribute("type", "0");
			Canm6To5SetQuaternionToEuler(inData, outData);
			break;
		}
		case 3: {
			Canm6To5SetFloatToUInt16(inData, outData);
			break;
		}
		default:
			outData->SetAttribute("type", "null");
			break;
		}
	}
}

void Canm6To5SetTransform(tinyxml2::XMLElement* inData, tinyxml2::XMLElement* outData)
{
	tinyxml2::XMLElement* inPtr;
	float f32_value[3];

	int i32_value = inData->IntAttribute("frame");
	outData->SetAttribute("frame", i32_value);

	inPtr = inData->FirstChildElement("initial");
	CanmResolverGetVector3(inPtr, f32_value);
	outData->SetAttribute("ix", f32_value[0]);
	outData->SetAttribute("iy", f32_value[1]);
	outData->SetAttribute("iz", f32_value[2]);

	inPtr = inData->FirstChildElement("velocity");
	CanmResolverGetVector3(inPtr, f32_value);
	outData->SetAttribute("vx", f32_value[0]);
	outData->SetAttribute("vy", f32_value[1]);
	outData->SetAttribute("vz", f32_value[2]);
}

void Canm6To5SetQuaternionToEuler(tinyxml2::XMLElement* inData, tinyxml2::XMLElement* outData)
{
	tinyxml2::XMLElement* inPtr;
	double f32_value[3];
	double f64_value[4];

	int i32_value = inData->IntAttribute("frame");
	outData->SetAttribute("frame", i32_value);

	inPtr = inData->FirstChildElement("initial");
	CanmResolverGetVector4(inPtr, f64_value);
	CanmResolverQuaternionToEuler(f64_value, f32_value);
	outData->SetAttribute("ix", (float)f32_value[0]);
	outData->SetAttribute("iy", (float)f32_value[1]);
	outData->SetAttribute("iz", (float)f32_value[2]);

	inPtr = inData->FirstChildElement("velocity");
	CanmResolverGetVector4(inPtr, f64_value);
	CanmResolverQuaternionToEuler(f64_value, f32_value);
	outData->SetAttribute("vx", (float)f32_value[0]);
	outData->SetAttribute("vy", (float)f32_value[1]);
	outData->SetAttribute("vz", (float)f32_value[2]);
}

void Canm6To5SetFloatToUInt16(tinyxml2::XMLElement* inData, tinyxml2::XMLElement* outData)
{
	std::vector< __m256d > v_KeyframeData;
	__m256d KeyframeData;
	KeyframeData.m256d_f64[3] = 1;
	double f64_value[4];

	for (tinyxml2::XMLElement* inKF = inData->FirstChildElement("keyframe")->FirstChildElement("v"); inKF != 0; inKF = inKF->NextSiblingElement("v")) {

		CanmResolverGetVector4(inKF, f64_value);
		CanmResolverQuaternionToEuler(f64_value, &KeyframeData.m256d_f64[0]);
		v_KeyframeData.push_back(KeyframeData);

	}

	int KFCount = v_KeyframeData.size();
	if (!KFCount) {
		outData->SetAttribute("type", "null");
		return;
	}
	outData->SetAttribute("type", "1");
	outData->SetAttribute("frame", KFCount);

	// get max and min value
	__m256d minKF = v_KeyframeData[0];
	__m256d maxKF = minKF;
	// no need to compare itself.
	for (int i = 1; i < KFCount; i++) {
		maxKF = _mm256_max_pd(maxKF, v_KeyframeData[i]);
		minKF = _mm256_min_pd(minKF, v_KeyframeData[i]);
	}

	// get value between max and min
	__m256d deltaKF = _mm256_sub_pd(maxKF, minKF);
	deltaKF = _mm256_div_pd(deltaKF, _mm256_set1_pd(0xFFFF));

	// set xml text
	outData->SetAttribute("ix", (float)minKF.m256d_f64[0]);
	outData->SetAttribute("iy", (float)minKF.m256d_f64[1]);
	outData->SetAttribute("iz", (float)minKF.m256d_f64[2]);
	outData->SetAttribute("vx", (float)deltaKF.m256d_f64[0]);
	outData->SetAttribute("vy", (float)deltaKF.m256d_f64[1]);
	outData->SetAttribute("vz", (float)deltaKF.m256d_f64[2]);

	// get keyframe/delta values
	for (int i = 0; i < KFCount; i++) {
		v_KeyframeData[i] = _mm256_sub_pd(v_KeyframeData[i], minKF);
		v_KeyframeData[i] = _mm256_div_pd(v_KeyframeData[i], deltaKF);
	}
	// set xml data text
	tinyxml2::XMLElement* xmlNode;
	UINT16 i16x3[3];
	for (int i = 0; i < KFCount; i++) {
		xmlNode = outData->InsertNewChildElement("v");
		i16x3[0] = v_KeyframeData[i].m256d_f64[0];
		i16x3[1] = v_KeyframeData[i].m256d_f64[1];
		i16x3[2] = v_KeyframeData[i].m256d_f64[2];
		CanmResolverSetVector3(xmlNode, i16x3);
	}
}

void CanmResolverGetVector3(tinyxml2::XMLElement* in, UINT16* vf)
{
	vf[0] = in->IntAttribute("x");
	vf[1] = in->IntAttribute("y");
	vf[2] = in->IntAttribute("z");
}

void CanmResolverGetVector3(tinyxml2::XMLElement* in, float* vf)
{
	vf[0] = in->FloatAttribute("x");
	vf[1] = in->FloatAttribute("y");
	vf[2] = in->FloatAttribute("z");
}

void CanmResolverGetVector3(tinyxml2::XMLElement* in, double* vf)
{
	vf[0] = in->DoubleAttribute("x");
	vf[1] = in->DoubleAttribute("y");
	vf[2] = in->DoubleAttribute("z");
}

void CanmResolverSetVector3(tinyxml2::XMLElement* out, const UINT16* vf)
{
	out->SetAttribute("x", vf[0]);
	out->SetAttribute("y", vf[1]);
	out->SetAttribute("z", vf[2]);
}

void CanmResolverGetVector4(tinyxml2::XMLElement* in, double* vf)
{
	vf[0] = in->DoubleAttribute("x");
	vf[1] = in->DoubleAttribute("y");
	vf[2] = in->DoubleAttribute("z");
	vf[3] = in->DoubleAttribute("w");
}

void CanmResolverQuaternionToEuler(const double* in, double* out)
{
	// do not adjust coordinate system
	double qx = in[0];
	double qy = in[1];
	double qz = in[2];
	double qw = in[3];

	// do not adjust output coordinate system
	// get x-axis£¨roll£©
	double sinr_cosp = 2 * (qw * qx + qy * qz);
	double cosr_cosp = 1 - 2 * (qx * qx + qy * qy);
	out[0] = std::atan2(sinr_cosp, cosr_cosp);

	// get y-axis£¨pitch£©
	double sinp = 2 * (qw * qy - qz * qx);
	if (std::abs(sinp) >= 1) {
		out[1] = std::copysign(M_PI / 2, sinp);
	}
	else {
		out[1] = std::asin(sinp);
	}

	// get z-axis£¨yaw£©
	double siny_cosp = 2 * (qw * qz + qx * qy);
	double cosy_cosp = 1 - 2 * (qy * qy + qz * qz);
	out[2] = std::atan2(siny_cosp, cosy_cosp);
}
