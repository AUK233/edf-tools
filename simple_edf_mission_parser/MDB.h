#pragma once
#include "include/tinyxml2.h"

struct MDBName
{
	std::wstring idname;
};

struct MDBBone
{
	int index[5];
};

struct MDBMaterial
{
	int matid;
	std::wstring shader;
	
	int PtrOffset;
	int PtrCount;
	// texture used, not name of texture
	int TexOffset;
	int TexCount;
};

struct MDBMaterialPtr
{
	float r;
	float g;
	float b;
	float a;
	std::string ptrname;
};

struct MDBMaterialTex
{
	int texid;
	std::string textype;
};

struct MDBObject
{
	int ID;
	int Nameid;
	int infoCount;
	int infoOffset;
};

struct MDBObjectInfo
{
	int matid;
	int LayoutOffset;
	uint16_t VertexSize;
	uint16_t LayoutCount;
	int VertexNum;
	int MeshIndex;
	int VertexOffset;
	int indicesNum;
	int indicesOffset;
};

struct MDBObjectLayout
{
	int type;
	int offset;
	int channel;
	std::string name;
};

struct MDBTexture
{
	/*
	MDBTexture(int num, std::wstring iname)
	{
		ID = num;
		filename = iname;
	}*/

	int ID;
	std::wstring mapping;
	std::wstring filename;
	std::string raw;
};

class CMDBtoXML
{
public:
	int Read(const std::wstring& path, bool onecore);

	MDBName ReadMDBName(int pos, std::vector<char> buffer);
	MDBTexture ReadTexture(int pos, std::vector<char> buffer);

	MDBBone ReadBone(int pos, std::vector<char> buffer);

	MDBMaterial ReadMaterial(int pos, std::vector<char> buffer);
	MDBMaterialPtr ReadMaterialPtr(int pos, std::vector<char> buffer);
	MDBMaterialTex ReadMaterialTex(int pos, std::vector<char> buffer);

	MDBObject ReadObject(int pos, std::vector<char> buffer);
	MDBObjectInfo ReadObjectInfo(int pos, std::vector<char> buffer);
	MDBObjectLayout ReadObjectLayout(int pos, std::vector<char> buffer);

	void ReadVertex(int pos, std::vector<char> buffer, int type, int num, int size, tinyxml2::XMLElement* header);

private:
	std::vector< MDBName > names;
	std::vector< MDBTexture > textures;

	std::vector< MDBBone > bones;

	std::vector< MDBMaterial > materials;
	std::vector< MDBMaterialPtr > materials_ptr;
	std::vector< MDBMaterialTex > materials_tex;

	std::vector< MDBObject > objects;
	std::vector< MDBObjectInfo > objects_info;
	std::vector< MDBObjectLayout > objects_layout;

	int NameTableCount;
	int NameTableOffset;
	int BoneCount;
	int BoneOffset;
	int ObjectCount;
	int ObjectOffset;
	int MaterialCount;
	int MaterialOffset;
	int TextureCount;
	int TextureOffset;
};

class CXMLToMDB
{
public:
	void Write(const std::wstring& path, bool multcore);
	void GenerateHeader(std::vector< char > &bytes);

	//Every wstring is counted (even if it is repeated!)
	int NameTableCount = 0;
	int BoneCount = 0;
	int ObjectCount = 0;
	int MaterialCount = 0;
	int TextureCount = 0;

private:

	std::vector< std::wstring > m_vecWNames;
	std::vector< std::wstring > m_vecTexArgs;
	std::vector< std::wstring > m_vecTexNames;
};