#pragma once
#include "include/tinyxml2.h"
#include <mutex>

struct MDBName
{
	std::wstring idname;
};

struct MDBBone
{
	int index[5];
	// write need
	unsigned char weight[3][4];
	float matrix1[4][4];
	float matrix2[4][4];
	float fg[2][4];
	std::vector< char > bytes;
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
	//To MDB use
	std::vector< char > bytes;
};

struct MDBMaterialPtr
{
	float r;
	float g;
	float b;
	float a;
	std::string ptrname;
	//To MDB use
	std::vector< char > bytes;
};

struct MDBMaterialTex
{
	int texid;
	std::string textype;
	//To MDB use
	std::vector< char > bytes;
};

struct MDBObject
{
	int ID;
	int Nameid;
	int infoCount;
	int infoOffset;
	//To MDB use
	std::vector< char > bytes;
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
	//To MDB use
	std::vector< char > bytes;
};

struct MDBObjectLayout
{
	int type;
	int offset;
	int channel;
	std::string name;
	//To MDB use
	std::vector< char > bytes;
};
//for writing to MDB
struct MDBObjectLayoutOut
{
	std::string name;
	std::vector< char > bytes;
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

	//To MDB use
	std::vector< char > bytes;
};

struct MDBByte
{
	std::vector< char > bytes;
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
	void ReadVertexMT(std::mutex &mtx, int pos, std::vector<char> buffer, int type, int num, int size, tinyxml2::XMLElement* header);

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
	void AlignFileTo16Bytes(std::vector<char>& bytes);
	void Set4BytesInFile(std::vector<char>& bytes, int pos, int value);
	void GenerateHeader(std::vector< char > &bytes);
	MDBTexture GetTexture(tinyxml2::XMLElement* entry2);
	MDBBone GetBone(tinyxml2::XMLElement* entry2, bool NoNameTable);
	void WriteRawToByte(std::string& argsStrn, std::vector<char> &buf, int pos);
	MDBMaterial GetMaterial(tinyxml2::XMLElement* entry2, bool NoNameTable, bool NoTexTable);
	MDBMaterialPtr GetMaterialParameter(tinyxml2::XMLElement* entry4);
	MDBMaterialTex GetMaterialTexture(tinyxml2::XMLElement* entry4, bool NoTexTable);
	MDBTexture GetTextureInMaterial(std::wstring wstr1, std::wstring wstr2);
	MDBObject GetModel(tinyxml2::XMLElement* entry2, bool NoNameTable, bool multcore);
	int GetMeshLayoutSize(tinyxml2::XMLElement* entry5);
	MDBObjectInfo GetMeshInModel(tinyxml2::XMLElement* entry3, int index, bool multcore);
	MDBObjectLayout GetLayoutInModel(tinyxml2::XMLElement* entry5, int layoutofs);
	MDBObjectLayoutOut GetLayoutInModel(MDBObjectLayout objlay, int size);
	MDBByte GetVerticesInModel(std::vector< MDBObjectLayout > objlay, int chunksize, tinyxml2::XMLElement* entry4, int num, int layout, bool multcore);
	void GetModelVertex(int type, int num, tinyxml2::XMLElement* entry5, std::vector< char > &bytes, int chunksize, int offset);
	MDBByte GetIndicesInModel(tinyxml2::XMLElement* entry4, int size);

	void WriteStringToTemp(std::string str);
	void WriteWStringToTemp(std::wstring wstr);

	//Every wide string is counted (even if it is repeated!)
	int NameTableCount = 0;
	//Real valid name!
	int NameCount = 0;
	int BoneCount = 0;
	int ObjectCount = 0;
	int MaterialCount = 0;
	int TextureCount = 0;

	//Store string
	std::vector< std::string > m_vecStrns;
	std::vector< std::wstring > m_vecWStrns;
	//Where to store the name table call string
	std::vector< int > m_vecNameTpos;
	//Where to store the texture call string
	std::vector< int > m_vecTexPos;
	//Where to store the material call string
	std::vector< int > m_vecMatPos;
	std::vector< int > m_vecMatPtrPos;
	std::vector< int > m_vecMatTexPos;
	//Where to store the model call string
	std::vector< int > m_vecModelPos;
	std::vector< int > m_vecObjInfoPos;
	std::vector< int > m_vecObjLayPos;

private:

	//wide string in name table
	std::vector< std::wstring > m_vecWNames;
	std::vector< MDBTexture > m_vecTexture;
	std::vector< MDBBone > m_vecBone;
	std::vector< MDBMaterial > m_vecMaterial;
	std::vector< MDBMaterialPtr > m_vecMaterialPtr;
	std::vector< MDBMaterialTex > m_vecMaterialTex;
	std::vector< MDBObject > m_vecObject;
	std::vector< MDBObjectInfo > m_vecObjInfo;
	std::vector< MDBObjectLayoutOut > m_vecObjLayout;
	std::vector< MDBByte > m_vecObjVertices;
	std::vector< MDBByte > m_vecObjIndices;
};