#pragma once
#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include <algorithm>

#include <iostream>
#include <locale>
#include <locale.h>
#include "util.h"
#include "ESB.h"

ESBHashToName_t ESBTable_RenderInfo[] = {
	{0x77ae72e2, "Life"},
	{0x2eab571, "Sprite"},
	{0x898a2569, "Texture"},
	{0x3389f874, "LocusExtendLine"},
	{0x547d366e, "ExtLine"},
	{0x120f6552, "Distortion"},
	{0x4ba8b053, "Null"},
	{0xbf77f8ea, "Line"},
	{0xc86f8310, "LocusLine"},
	{0xd1fa7db6, "EmittingExtendLine"},
	{0xf8946d12, "Model"}
};

ESBHashToName_t ESBTable_Type[] = {
	{0x2194205d, "Singleton"},
	{0x4acb332b, "Emitter"}
};

ESBHashToName_t ESBTable_Particle[] = {
	{0x46a3f14f, "Particle3D"},
	{0x5fb8c00e, "Particle2D"}
};

ESBHashToName_t ESBTable_Transform[] = {
	{0x463acce, "RootTransform"},
	{0x651b6c6, "CameraTransform"},
	{0x11f606a2, "BillboardTransform"},
	{0x33709f42, "Transform"},
	{0xb9863f02, "BoneTransform"}
};

ESBHashToName_t ESBTable_Translation[] = {
	{0x11093d22, "LocalXYZ"},
	{0x9a1c226c, "Fixed"},
	{0xfa4d2e4f, "XYZ"},
	{0x4d795c62, "XY"}
};

ESBHashToName_t ESBTable_Scale[] = {
	{0x4a35f382, "AllFixed"},
	{0x6c5b6a63, "All"},
	{0xbccda1d9, "XYZ"},
	{0xc000d7c1, "Fixed"}
};

ESBHashToName_t ESBTable_Rotation[] = {
	{0x32a00c44, "XYZ"},
	{0xac8872ae, "Fixed"},
	{0xda9ea8ee, "Billboard"},
	{0xec83d752, "Z"}
};

ESBHashToName_t ESBTable_Color[] = {
	{0x1998c0df, "RGBA"},
	{0xc1ff7551, "A"}
};

ESBHashToName_t ESBTable_Mesh[] = {
	{0x62c96017, "Quad"},
	{0x7bd1d898, "Ring"}
};

ESBHashToName_t ESBTable_Shape[] = {
	{0x8f99c2d3, "Cylinder"},
	{0x8f99c2d3, "Sphere"},
	{0xbd515c14, "Plane"},
	{0xf8fd7dfd, "Box"}
};

ESBHashToName_t ESBTable_Light[] = {
	{0x87c0bcf0, "SpotLight"},
	{0xb420d6b1, "PointLight"},
	{0xf7cca492, "BarLight"},
};
