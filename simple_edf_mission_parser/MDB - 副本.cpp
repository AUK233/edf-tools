#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <codecvt>
#include <sstream>

#include <iostream>
#include <locale>
#include "util.h"
#include "MDB.h"
#include "include/tinyxml2.h"

int CMDBtoXML::Read(const std::wstring& path)
{
	std::ifstream file(path + L".mdb", std::ios::binary | std::ios::ate);

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	if (file.read(buffer.data(), size))
	{
		unsigned char seg[4];

		int position = 0;

		Read4BytesReversed(seg, buffer, position);

		bool validHeader = false;
		if (seg[0] == 0x4D && seg[1] == 0x44 && seg[2] == 0x42 && seg[3] == 0x30)
			validHeader = true;

		if (!validHeader)
		{
			std::wcout << L"BAD FILE\n";
			file.close();
			return -1;
		}

		tinyxml2::XMLDocument xml = new tinyxml2::XMLDocument();
		xml.InsertFirstChild(xml.NewDeclaration());
		tinyxml2::XMLElement* xmlHeader = xml.NewElement("MDB");
		xml.InsertEndChild(xmlHeader);
		
		std::ofstream output(path + L"_MDB.xml", std::ios::binary | std::ios::out | std::ios::ate );
		/*
		std::locale utf8_locale(output.getloc(), new std::codecvt_utf8<wchar_t>);
		output.imbue(utf8_locale);*/

		//Parse the header

		// Name
		position = 0x8;
		Read4Bytes(seg, buffer, position);
		NameTableCount = GetIntFromChunk(seg);

		position = 0x0C;
		Read4Bytes(seg, buffer, position);
		NameTableOffset = GetIntFromChunk(seg);

		// Bone
		position = 0x10;
		Read4Bytes(seg, buffer, position);
		BoneCount = GetIntFromChunk(seg);

		position = 0x14;
		Read4Bytes(seg, buffer, position);
		BoneOffset = GetIntFromChunk(seg);

		// Object
		position = 0x18;
		Read4Bytes(seg, buffer, position);
		ObjectCount = GetIntFromChunk(seg);

		position = 0x1C;
		Read4Bytes(seg, buffer, position);
		ObjectOffset = GetIntFromChunk(seg);

		// Material
		position = 0x20;
		Read4Bytes(seg, buffer, position);
		MaterialCount = GetIntFromChunk(seg);

		position = 0x24;
		Read4Bytes(seg, buffer, position);
		MaterialOffset = GetIntFromChunk(seg);

		// Object
		position = 0x28;
		Read4Bytes(seg, buffer, position);
		TextureCount = GetIntFromChunk(seg);

		position = 0x2C;
		Read4Bytes(seg, buffer, position);
		TextureOffset = GetIntFromChunk(seg);


		//Read
		// name table:
		if (NameTableCount > 0)
		{
			tinyxml2::XMLElement* xmlName = xmlHeader->InsertNewChildElement("Names");
			xmlName->SetAttribute("debug_allcount", NameTableCount);

			std::string utf8str; 
			for (int i = 0; i < NameTableCount; i++)
			{
				int curtablepos = NameTableOffset + (i * 0x4);

				names.push_back(ReadMDBName(curtablepos, buffer));
				// write to file
				if (names.back().idname != L"null")
				{
					utf8str = WideToUTF8(names.back().idname);

					tinyxml2::XMLElement* xmlNameVal = xmlName->InsertNewChildElement("value");
					xmlNameVal->SetText(utf8str.c_str());
					xmlNameVal->SetAttribute("index", i);
				}
			}
		}
		// texture table:
		if (TextureCount > 0)
		{
			tinyxml2::XMLElement* xmlTex = xmlHeader->InsertNewChildElement("Textures");
			xmlTex->SetAttribute("count", TextureCount);

			std::string utf8str;
			for (int i = 0; i < TextureCount; i++)
			{
				int curtablepos = TextureOffset + (i * 0x10);

				textures.push_back(ReadTexture(curtablepos, buffer));

				utf8str = std::to_string(textures.back().ID) + ", \"";
				utf8str += WideToUTF8(textures.back().mapping) + "\", \"";
				utf8str += WideToUTF8(textures.back().filename) + "\", ";
				utf8str += textures.back().raw;
				
				tinyxml2::XMLElement* xmlTexVal = xmlTex->InsertNewChildElement("value");
				xmlTexVal->SetText(utf8str.c_str());
			}
			//xmlTexVal->InsertNewText(utf8str.c_str());
		}
		// material table:
		if (MaterialCount > 0)
		{
			tinyxml2::XMLElement* xmlMatHeader = xmlHeader->InsertNewChildElement("Materials");
			xmlMatHeader->SetAttribute("count", MaterialCount);

			std::string utf8str; 
			for (int i = 0; i < MaterialCount; i++)
			{
				tinyxml2::XMLElement* xmlMat = xmlMatHeader->InsertNewChildElement("MaterialNode");

				int curtablepos = MaterialOffset + (i * 0x20);
				//Raw hex 1
				utf8str = ReadRaw(buffer, curtablepos, 4);
				tinyxml2::XMLElement* xmlMatRaw1 = xmlMat->InsertNewChildElement("raw");
				xmlMatRaw1->SetText(utf8str.c_str());
				xmlMatRaw1->SetAttribute("inPos", curtablepos);
				//known data:
				materials.push_back(ReadMaterial(curtablepos, buffer));

				int tempint = materials.back().matid;
				tinyxml2::XMLElement* xmlMatID = xmlMat->InsertNewChildElement("MaterialName");
				xmlMatID->SetAttribute("index", i);
				xmlMatID->SetAttribute("MatID", tempint);
				utf8str = WideToUTF8(names[tempint].idname);
				xmlMatID->SetText(utf8str.c_str());

				tinyxml2::XMLElement* xmlMatSdr = xmlMat->InsertNewChildElement("Shader");
				utf8str = WideToUTF8(materials.back().shader);
				xmlMatSdr->SetAttribute("Name", utf8str.c_str());
				//Raw hex 2
				utf8str = ReadRaw(buffer, curtablepos + 0x1C, 4);
				tinyxml2::XMLElement* xmlMatRaw2 = xmlMat->InsertNewChildElement("raw");
				xmlMatRaw2->SetText(utf8str.c_str());
				xmlMatRaw2->SetAttribute("inPos", curtablepos + 0x1C);
				//Parse shader parameters
				for (int j = 0; j < materials.back().PtrCount; j++)
				{
					tinyxml2::XMLElement* xmlMatPtr = xmlMatSdr->InsertNewChildElement("Parameter");

					int curpos = curtablepos + materials.back().PtrOffset + (j * 0x20);

					materials_ptr.push_back(ReadMaterialPtr(curpos, buffer));

					xmlMatPtr->SetAttribute("Name", materials_ptr.back().ptrname.c_str());

					utf8str = std::to_string(materials_ptr.back().r) + ", ";
					utf8str += std::to_string(materials_ptr.back().g) + ", ";
					utf8str += std::to_string(materials_ptr.back().b) + ", ";
					utf8str += std::to_string(materials_ptr.back().a);
					tinyxml2::XMLElement* xmlMatPtrVal = xmlMatPtr->InsertNewChildElement("Color");
					xmlMatPtrVal->SetText(utf8str.c_str());

					//Raw hex 1
					utf8str = ReadRaw(buffer, curpos + 0x10, 8);
					tinyxml2::XMLElement* xmlMatPtrRaw1 = xmlMatPtr->InsertNewChildElement("raw");
					xmlMatPtrRaw1->SetText(utf8str.c_str());
					xmlMatPtrRaw1->SetAttribute("inPos", curpos + 0x10);
					//Raw hex 2
					utf8str = ReadRaw(buffer, curpos + 0x1C, 4);
					tinyxml2::XMLElement* xmlMatPtrRaw2 = xmlMatPtr->InsertNewChildElement("raw");
					xmlMatPtrRaw2->SetText(utf8str.c_str());
					xmlMatPtrRaw2->SetAttribute("inPos", curpos + 0x1C);
				}
				//Parse the texture used
				for (int k = 0; k < materials.back().TexCount; k++)
				{
					tinyxml2::XMLElement* xmlMatTex = xmlMatSdr->InsertNewChildElement("Texture");
					xmlMatTex->SetAttribute("index", k);

					int curpos = curtablepos + materials.back().TexOffset + (k * 0x1C);

					materials_tex.push_back(ReadMaterialTex(curpos, buffer));

					tempint = materials_tex.back().texid;
					tinyxml2::XMLElement* xmlMatTexID = xmlMatTex->InsertNewChildElement("Name");
					xmlMatTexID->SetAttribute("MatID", tempint);
					utf8str = WideToUTF8(textures[tempint].filename);
					xmlMatTexID->SetText(utf8str.c_str());

					tinyxml2::XMLElement* xmlMatTexType = xmlMatTex->InsertNewChildElement("Type");
					xmlMatTexType->SetText(materials_tex.back().textype.c_str());

					//Raw hex
					utf8str = ReadRaw(buffer, curpos + 0x8, 20);
					tinyxml2::XMLElement* xmlMatTexRaw = xmlMatTex->InsertNewChildElement("raw");
					xmlMatTexRaw->SetText(utf8str.c_str());
					xmlMatTexRaw->SetAttribute("inPos", curpos + 0x8);
				}
			}
		}
		// get bone list
		if (BoneCount > 0)
		{
			tinyxml2::XMLElement* xmlBoneList = xmlHeader->InsertNewChildElement("BoneLists");
			xmlBoneList->SetAttribute("count", BoneCount);

			std::string utf8str; 
			for (int i = 0; i < BoneCount; i++)
			{
				tinyxml2::XMLElement* xmlBone = xmlBoneList->InsertNewChildElement("Bone");

				int curtablepos = BoneOffset + (i * 0xC0);

				bones.push_back(ReadBone(curtablepos, buffer));

				tinyxml2::XMLElement* xmlBoneI = xmlBone->InsertNewChildElement("value");
				xmlBoneI->SetText(bones.back().index);
				tinyxml2::XMLElement* xmlBoneP = xmlBone->InsertNewChildElement("parent");
				xmlBoneP->SetText(bones.back().parent);

				//Raw hex 1
				utf8str = ReadRaw(buffer, curtablepos + 0x8, 8);
				tinyxml2::XMLElement* xmlBoneRaw1 = xmlBone->InsertNewChildElement("raw");
				xmlBoneRaw1->SetText(utf8str.c_str());
				xmlBoneRaw1->SetAttribute("inPos", curtablepos + 0x8);
				//data
				tinyxml2::XMLElement* xmlBoneID = xmlBone->InsertNewChildElement("name");
				int tempint = bones.back().id;
				xmlBoneID->SetAttribute("id", tempint);
				utf8str = WideToUTF8(names[tempint].idname);
				xmlBoneID->SetText(utf8str.c_str());
				//Raw hex
				for (int j = 0;j<10;j++)
				{
					int tpos = curtablepos + 0x14 + (j*0x10);
					
					utf8str = ReadRaw(buffer, tpos, 0x10);
					tinyxml2::XMLElement* xmlBoneRaw = xmlBone->InsertNewChildElement("raw");
					xmlBoneRaw->SetText(utf8str.c_str());
					xmlBoneRaw->SetAttribute("inPos", tpos);
				}
				//Raw hex 2
				utf8str = ReadRaw(buffer, curtablepos + 180, 12);
				tinyxml2::XMLElement* xmlBoneRaw2 = xmlBone->InsertNewChildElement("raw");
				xmlBoneRaw2->SetText(utf8str.c_str());
				xmlBoneRaw2->SetAttribute("inPos", curtablepos + 180);
			}
		}
		// get object list
		if (ObjectCount > 0)
		{
			tinyxml2::XMLElement* xmlObjList = xmlHeader->InsertNewChildElement("ObjectLists");
			xmlObjList->SetAttribute("count", ObjectCount);

			std::string utf8str; 
			for (int i = 0; i < ObjectCount; i++)
			{
				tinyxml2::XMLElement* xmlObj = xmlObjList->InsertNewChildElement("Object");

				int curtablepos = ObjectOffset + (i * 0x10);

				objects.push_back(ReadObject(curtablepos, buffer));

				int tempint = objects.back().Nameid;

				xmlObj->SetAttribute("ID", objects.back().ID);
				xmlObj->SetAttribute("NameID", tempint);

				tinyxml2::XMLElement* xmlObjName = xmlObj->InsertNewChildElement("name");
				utf8str = WideToUTF8(names[tempint].idname);
				xmlObjName->SetText(utf8str.c_str());

				xmlObjList->SetAttribute("count", objects.back().infoCount);
				// get mesh info
				for (int j = 0; j < objects.back().infoCount; j++)
				{
					tinyxml2::XMLElement* xmlMesh = xmlObj->InsertNewChildElement("Mesh");

					int curpos = curtablepos + objects.back().infoOffset + (j * 0x28);

					objects_info.push_back(ReadObjectInfo(curpos, buffer));

					//Raw hex 1
					utf8str = ReadRaw(buffer, curpos, 4);
					tinyxml2::XMLElement* xmlMeshRaw1 = xmlMesh->InsertNewChildElement("raw");
					xmlMeshRaw1->SetText(utf8str.c_str());
					xmlMeshRaw1->SetAttribute("inPos", curpos);
					//Material
					xmlMesh->SetAttribute("MatID",  objects_info.back().matid);
					xmlMesh->SetAttribute("MeshIndex",  objects_info.back().MeshIndex);
					//Raw hex 2
					utf8str = ReadRaw(buffer, curpos + 0x8, 4);
					tinyxml2::XMLElement* xmlMeshRaw2 = xmlMesh->InsertNewChildElement("raw");
					xmlMeshRaw2->SetText(utf8str.c_str());
					xmlMeshRaw2->SetAttribute("inPos", curpos + 0x8);
					//Data
					tinyxml2::XMLElement* xmlLayout = xmlMesh->InsertNewChildElement("Layout");
					xmlLayout->SetAttribute("Count",  objects_info.back().LayoutCount);
					//Read Layout Info
					for (int k = 0; k < objects_info.back().LayoutCount; k++)
					{
						int newcurpos = curpos + objects_info.back().LayoutOffset + (k*0x10);

						objects_layout.push_back(ReadObjectLayout(newcurpos, buffer));

						tinyxml2::XMLElement* xmlNode = xmlLayout->InsertNewChildElement("Value");

						xmlNode->SetAttribute("type", objects_layout.back().type);
						xmlNode->SetAttribute("offset", objects_layout.back().offset);
						xmlNode->SetAttribute("channel", objects_layout.back().channel);
						xmlNode->SetAttribute("name", objects_layout.back().name.c_str());

						xmlNode->SetAttribute("debugIndex", k);
					}
					//Read Vertex
					tinyxml2::XMLElement* xmlVertexList = xmlMesh->InsertNewChildElement("VertexList");
					xmlVertexList->SetAttribute("Count",  objects_info.back().VertexNum);
					for (int k = 0; k < objects_info.back().VertexNum; k++)
					{
						int newcurpos = curpos + objects_info.back().VertexOffset + (k * objects_info.back().VertexSize);

						tinyxml2::XMLElement* xmlVertex = xmlVertexList->InsertNewChildElement("Vertex");
						for (int l = 0; l < objects_info.back().LayoutCount; l++)
						{
							int Vtype = objects_layout[l].type;
							int Vcurpos = newcurpos + objects_layout[l].offset;
							std::string Vstr = objects_layout[l].name;

							if (Vtype == 1)
							{
								tinyxml2::XMLElement* xmlNode = xmlVertex->InsertNewChildElement(Vstr.c_str());
								float vf;
								unsigned char seg[4];

								Read4BytesReversed(seg, buffer, Vcurpos);
								memcpy(&vf, &seg, sizeof(vf));
								xmlNode->SetAttribute("x", vf);
								Read4BytesReversed(seg, buffer, Vcurpos + 0x4);
								memcpy(&vf, &seg, sizeof(vf));
								xmlNode->SetAttribute("y", vf);
								Read4BytesReversed(seg, buffer, Vcurpos + 0x8);
								memcpy(&vf, &seg, sizeof(vf));
								xmlNode->SetAttribute("z", vf);
								Read4BytesReversed(seg, buffer, Vcurpos + 0x0C);
								memcpy(&vf, &seg, sizeof(vf));
								xmlNode->SetAttribute("w", vf);
							}
							else if (Vtype == 4)
							{
								tinyxml2::XMLElement* xmlNode = xmlVertex->InsertNewChildElement(Vstr.c_str());
								float vf;
								unsigned char seg[4];

								Read4BytesReversed(seg, buffer, Vcurpos);
								memcpy(&vf, &seg, sizeof(vf));
								xmlNode->SetAttribute("x", vf);
								Read4BytesReversed(seg, buffer, Vcurpos + 0x4);
								memcpy(&vf, &seg, sizeof(vf));
								xmlNode->SetAttribute("y", vf);
								Read4BytesReversed(seg, buffer, Vcurpos + 0x8);
								memcpy(&vf, &seg, sizeof(vf));
								xmlNode->SetAttribute("z", vf);
							}
							else if (Vtype == 7)
							{
								tinyxml2::XMLElement* xmlNode = xmlVertex->InsertNewChildElement(Vstr.c_str());
								float vf;

								vf = ReadHalfFloat(buffer, Vcurpos);
								xmlNode->SetAttribute("x", vf);
								vf = ReadHalfFloat(buffer, Vcurpos + 0x2);
								xmlNode->SetAttribute("y", vf);
								vf = ReadHalfFloat(buffer, Vcurpos + 0x4);
								xmlNode->SetAttribute("z", vf);
								vf = ReadHalfFloat(buffer, Vcurpos + 0x6);
								xmlNode->SetAttribute("w", vf);
							}
							else if (Vtype == 12)
							{
								tinyxml2::XMLElement* xmlNode = xmlVertex->InsertNewChildElement(Vstr.c_str());
								float vf;
								unsigned char seg[4];

								Read4BytesReversed(seg, buffer, Vcurpos);
								memcpy(&vf, &seg, sizeof(vf));
								xmlNode->SetAttribute("x", vf);
								Read4BytesReversed(seg, buffer, Vcurpos+0x4);
								memcpy(&vf, &seg, sizeof(vf));
								xmlNode->SetAttribute("y", vf);
							}
							else if (Vtype == 21)
							{
								tinyxml2::XMLElement* xmlNode = xmlVertex->InsertNewChildElement(Vstr.c_str());
								float vf;
								unsigned char seg[4];

								Read4BytesReversed(seg, buffer, Vcurpos);
								xmlNode->SetAttribute("x", seg[0]);
								xmlNode->SetAttribute("y", seg[1]);
								xmlNode->SetAttribute("z", seg[2]);
								xmlNode->SetAttribute("w", seg[3]);
							}
						}
					}
					//Read indices
					tinyxml2::XMLElement* xmlIndices = xmlMesh->InsertNewChildElement("Indices");
					xmlIndices->SetAttribute("Count", objects_info.back().indicesNum);
					for (int k = 0; k < objects_info.back().indicesNum; k++)
					{
						int newcurpos = curpos + objects_info.back().indicesOffset + (k*2);
						tinyxml2::XMLElement* xmlNode = xmlIndices->InsertNewChildElement("value");
						//short int16 = ReadInt16(buffer, newcurpos);
						xmlNode->SetText(ReadInt16(buffer, newcurpos));
					}
				}
				//mark 3
			}
			//mark 2
		}


		tinyxml2::XMLPrinter printer;
		xml.Accept(&printer);
		auto xmlString = std::string{ printer.CStr() };

		//std::wcout << UTF8ToWide(xmlString);
		FindAndReplaceAll(xmlString, "\n", "\r\n");
		output << xmlString;
		output.close();

		file.close();
	}
}

MDBName CMDBtoXML::ReadMDBName(int pos, std::vector<char> buffer)
{
	MDBName out;

	unsigned char seg[4];
	int offset = 0;

	int position = pos;
	Read4Bytes(seg, buffer, position);
	offset = GetIntFromChunk(seg);
	if (offset > 0)
		out.idname = ReadUnicode(buffer, pos + offset);
	else
		out.idname = L"null";

	return out;
}

MDBBone CMDBtoXML::ReadBone(int pos, std::vector<char> buffer)
{
	MDBBone out;

	unsigned char seg[4];
	int offset = 0;

	int position = pos;
	Read4Bytes(seg, buffer, position);
	out.index = GetIntFromChunk(seg);

	position = pos + 0x4;
	Read4Bytes(seg, buffer, position);
	out.parent = GetIntFromChunk(seg);

	position = pos + 0x10;
	Read4Bytes(seg, buffer, position);
	out.id = GetIntFromChunk(seg);

	return out;
}

MDBMaterial CMDBtoXML::ReadMaterial(int pos, std::vector<char> buffer)
{
	MDBMaterial out;

	unsigned char seg[4];
	int offset = 0;

	int position = pos + 0x4;
	Read4Bytes(seg, buffer, position);
	out.matid = GetIntFromChunk(seg);

	position = pos + 0x8;
	Read4Bytes(seg, buffer, position);
	offset = GetIntFromChunk(seg);
	out.shader = ReadUnicode(buffer, pos + offset);
	// next
	position = pos + 0x0C;
	Read4Bytes(seg, buffer, position);
	out.PtrOffset = GetIntFromChunk(seg);

	position = pos + 0x10;
	Read4Bytes(seg, buffer, position);
	out.PtrCount = GetIntFromChunk(seg);

	position = pos + 0x14;
	Read4Bytes(seg, buffer, position);
	out.TexOffset = GetIntFromChunk(seg);

	position = pos + 0x18;
	Read4Bytes(seg, buffer, position);
	out.TexCount = GetIntFromChunk(seg);

	return out;
}

MDBMaterialPtr CMDBtoXML::ReadMaterialPtr(int pos, std::vector<char> buffer)
{
	MDBMaterialPtr out;

	unsigned char seg[4];
	float f;
	int offset = 0;

	int position = pos;
	Read4BytesReversed(seg, buffer, position);
	memcpy(&f, &seg, sizeof(f));
	out.r = f;

	position = pos + 0x4;
	Read4BytesReversed(seg, buffer, position);
	memcpy(&f, &seg, sizeof(f));
	out.g = f;

	position = pos + 0x8;
	Read4BytesReversed(seg, buffer, position);
	memcpy(&f, &seg, sizeof(f));
	out.b = f;

	position = pos + 0x0C;
	Read4BytesReversed(seg, buffer, position);
	memcpy(&f, &seg, sizeof(f));
	out.a = f;

	position = pos + 0x18;
	Read4Bytes(seg, buffer, position);
	offset = GetIntFromChunk(seg);
	out.ptrname = ReadASCII(buffer, pos + offset);

	return out;
}

MDBMaterialTex CMDBtoXML::ReadMaterialTex(int pos, std::vector<char> buffer)
{
	MDBMaterialTex out;

	unsigned char seg[4];
	int offset = 0;

	int position = pos;
	Read4Bytes(seg, buffer, position);
	out.texid = GetIntFromChunk(seg);

	position = pos + 0x4;
	Read4Bytes(seg, buffer, position);
	offset = GetIntFromChunk(seg);
	out.textype = ReadASCII(buffer, pos + offset);

	return out;
}

MDBObject CMDBtoXML::ReadObject(int pos, std::vector<char> buffer)
{
	MDBObject out;

	unsigned char seg[4];
	int offset = 0;

	int position = pos;
	Read4Bytes(seg, buffer, position);
	out.ID = GetIntFromChunk(seg);

	position = pos + 0x4;
	Read4Bytes(seg, buffer, position);
	out.Nameid = GetIntFromChunk(seg);

	position = pos + 0x8;
	Read4Bytes(seg, buffer, position);
	out.infoCount = GetIntFromChunk(seg);

	position = pos + 0x0C;
	Read4Bytes(seg, buffer, position);
	out.infoOffset = GetIntFromChunk(seg);

	return out;
}

MDBObjectInfo CMDBtoXML::ReadObjectInfo(int pos, std::vector<char> buffer)
{
	MDBObjectInfo out;

	unsigned char seg[4];
	int offset = 0;

	int position = pos + 0x4;
	Read4Bytes(seg, buffer, position);
	out.matid = GetIntFromChunk(seg);

	position = pos + 0x0C;
	Read4Bytes(seg, buffer, position);
	out.LayoutOffset = GetIntFromChunk(seg);

	position = pos + 0x10;
	out.VertexSize = ReadInt16(buffer, position);
	position = pos + 0x12;
	out.LayoutCount = ReadInt16(buffer, position);

	position = pos + 0x14;
	Read4Bytes(seg, buffer, position);
	out.VertexNum = GetIntFromChunk(seg);

	position = pos + 0x18;
	Read4Bytes(seg, buffer, position);
	out.MeshIndex = GetIntFromChunk(seg);

	position = pos + 0x1C;
	Read4Bytes(seg, buffer, position);
	out.VertexOffset = GetIntFromChunk(seg);

	position = pos + 0x20;
	Read4Bytes(seg, buffer, position);
	out.indicesNum = GetIntFromChunk(seg);

	position = pos + 0x24;
	Read4Bytes(seg, buffer, position);
	out.indicesOffset = GetIntFromChunk(seg);

	return out;
}

MDBObjectLayout CMDBtoXML::ReadObjectLayout(int pos, std::vector<char> buffer)
{
	MDBObjectLayout out;

	unsigned char seg[4];

	int position = pos;
	Read4Bytes(seg, buffer, position);
	out.type = GetIntFromChunk(seg);

	position = pos + 0x4;
	Read4Bytes(seg, buffer, position);
	out.offset = GetIntFromChunk(seg);

	position = pos + 0x8;
	Read4Bytes(seg, buffer, position);
	out.channel = GetIntFromChunk(seg);

	position = pos + 0x0C;
	Read4Bytes(seg, buffer, position);
	int offset = GetIntFromChunk(seg);
	out.name = ReadASCII(buffer, pos + offset);

	return out;
}

MDBTexture CMDBtoXML::ReadTexture(int pos, std::vector<char> buffer)
{
	MDBTexture out;

	unsigned char seg[4];
	int offset = 0;

	int position = pos;
	Read4Bytes(seg, buffer, position);
	out.ID = GetIntFromChunk(seg);

	position = pos + 0x4;
	Read4Bytes(seg, buffer, position);
	offset = GetIntFromChunk(seg);
	out.mapping = ReadUnicode(buffer, pos + offset);

	position = pos + 0x8;
	Read4Bytes(seg, buffer, position);
	offset = GetIntFromChunk(seg);
	out.filename = ReadUnicode(buffer, pos + offset);
	
	position = pos + 0xC;
	out.raw = ReadRaw(buffer, position, 4);

	return out;
}