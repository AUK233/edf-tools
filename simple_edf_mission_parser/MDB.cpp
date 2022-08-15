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
#include "include/half.hpp"

#include <thread>
#include <mutex>

int CMDBtoXML::Read(const std::wstring& path, bool onecore)
{
	std::ifstream file(path + L".mdb", std::ios::binary | std::ios::ate | std::ios::in);

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

		// Read
		// name table:
		if (NameTableCount > 0)
		{
			tinyxml2::XMLElement* xmlName = xmlHeader->InsertNewChildElement("Names");
			xmlName->SetAttribute("debug_allcount", NameTableCount);

			std::wcout << L"Getting name list...... ";

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
			std::wcout << L"Completed!\n";
		}
		// texture table:
		if (TextureCount > 0)
		{
			tinyxml2::XMLElement* xmlTex = xmlHeader->InsertNewChildElement("Textures");
			xmlTex->SetAttribute("count", TextureCount);

			std::wcout << L"Getting texture list...... ";

			std::string utf8str;
			for (int i = 0; i < TextureCount; i++)
			{
				int curtablepos = TextureOffset + (i * 0x10);

				textures.push_back(ReadTexture(curtablepos, buffer));
				// Don't use it now
				/*
				utf8str = std::to_string(textures.back().ID) + ", \"";
				utf8str += WideToUTF8(textures.back().mapping) + "\", \"";
				utf8str += WideToUTF8(textures.back().filename) + "\", ";
				utf8str += textures.back().raw;
				xmlTexVal->SetText(utf8str.c_str());
				*/
				tinyxml2::XMLElement* xmlTexVal = xmlTex->InsertNewChildElement("value");
				xmlTexVal->SetAttribute("ID", textures.back().ID);
				utf8str = WideToUTF8(textures.back().mapping);
				xmlTexVal->SetAttribute("mapping", utf8str.c_str());
				utf8str = WideToUTF8(textures.back().filename);
				xmlTexVal->SetAttribute("filename", utf8str.c_str());
				utf8str = textures.back().raw;
				xmlTexVal->SetAttribute("raw", utf8str.c_str());
			}
			//xmlTexVal->InsertNewText(utf8str.c_str());
			std::wcout << L"Completed!\n";
		}
		// get bone list
		if (BoneCount > 0)
		{
			tinyxml2::XMLElement* xmlBoneList = xmlHeader->InsertNewChildElement("BoneLists");
			xmlBoneList->SetAttribute("count", BoneCount);

			std::wcout << L"Getting bone list...... ";

			std::string utf8str; 
			for (int i = 0; i < BoneCount; i++)
			{
				tinyxml2::XMLElement* xmlBone = xmlBoneList->InsertNewChildElement("Bone");

				int curtablepos = BoneOffset + (i * 0xC0);

				bones.push_back(ReadBone(curtablepos, buffer));
				//treat the first value as a name
				tinyxml2::XMLElement* xmlBoneID = xmlBone->InsertNewChildElement("name");
				int tempint = bones.back().index[0];
				xmlBoneID->SetAttribute("id", tempint);
				utf8str = WideToUTF8(names[tempint].idname);
				xmlBoneID->SetText(utf8str.c_str());
				
				tinyxml2::XMLElement* xmlBoneP = xmlBone->InsertNewChildElement("parent");
				xmlBoneP->SetText(bones.back().index[1]);
				//think of these values as a special link
				tinyxml2::XMLElement* xmlBoneIK = xmlBone->InsertNewChildElement("IK");
				xmlBoneIK->SetAttribute("root", bones.back().index[2]);
				xmlBoneIK->SetAttribute("next", bones.back().index[3]);
				xmlBoneIK->SetAttribute("current", bones.back().index[4]);

				int tpos;
				//Read bone weights, 3 groups?
				for (int j = 0; j < 3; j++)
				{
					tpos = curtablepos + 0x14 + (j * 0x4);

					tinyxml2::XMLElement* xmlBNode = xmlBone->InsertNewChildElement("weight");
					unsigned char seg[4];

					Read4BytesReversed(seg, buffer, tpos);
					xmlBNode->SetAttribute("x", seg[0]);
					xmlBNode->SetAttribute("y", seg[1]);
					xmlBNode->SetAttribute("z", seg[2]);
					xmlBNode->SetAttribute("w", seg[3]);
				}
				//Read Float
				for (int j = 0; j < 10; j++)
				{
					tpos = curtablepos + 0x20 + (j * 0x10);

					tinyxml2::XMLElement* xmlBNode = xmlBone->InsertNewChildElement("float");
					float bf;
					unsigned char seg[4];

					Read4BytesReversed(seg, buffer, tpos);
					memcpy(&bf, &seg, 4U);
					xmlBNode->SetAttribute("x", bf);
					Read4BytesReversed(seg, buffer, tpos + 0x4);
					memcpy(&bf, &seg, 4U);
					xmlBNode->SetAttribute("y", bf);
					Read4BytesReversed(seg, buffer, tpos + 0x8);
					memcpy(&bf, &seg, 4U);
					xmlBNode->SetAttribute("z", bf);
					Read4BytesReversed(seg, buffer, tpos + 0xC);
					memcpy(&bf, &seg, 4U);
					xmlBNode->SetAttribute("w", bf);

					utf8str = ReadRaw(buffer, tpos, 0x10);
					xmlBNode->SetText(utf8str.c_str());
					xmlBNode->SetAttribute("debugPos", tpos);
				}
				//Raw hex
				/*
				for (int j = 0;j<10;j++)
				{
					int tpos = curtablepos + 0x14 + (j*0x10);
					
					utf8str = ReadRaw(buffer, tpos, 0x10);
					tinyxml2::XMLElement* xmlBoneRaw = xmlBone->InsertNewChildElement("raw");
					xmlBoneRaw->SetText(utf8str.c_str());
					xmlBoneRaw->SetAttribute("inPos", tpos);
				}
				utf8str = ReadRaw(buffer, curtablepos + 180, 12);
				tinyxml2::XMLElement* xmlBoneRaw2 = xmlBone->InsertNewChildElement("raw");
				xmlBoneRaw2->SetText(utf8str.c_str());
				xmlBoneRaw2->SetAttribute("inPos", curtablepos + 180);
				*/
			}
			std::wcout << L"Completed!\n\n";
		}
		// get object list
		if (ObjectCount > 0)
		{
			tinyxml2::XMLElement* xmlObjList = xmlHeader->InsertNewChildElement("ObjectLists");
			xmlObjList->SetAttribute("count", ObjectCount);

			std::wcout << L"Getting model list:\n";

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

				xmlObj->SetAttribute("count", objects.back().infoCount);
				std::wcout << L"Model parsing:" + names[tempint].idname + L"\n";
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
					int Layoutnum = objects_info.back().LayoutCount;
					tinyxml2::XMLElement* xmlLayout = xmlMesh->InsertNewChildElement("Layout");
					xmlLayout->SetAttribute("Count", Layoutnum);

					int Vnum = objects_info.back().VertexNum;
					tinyxml2::XMLElement* xmlVertexList = xmlMesh->InsertNewChildElement("VertexList");
					xmlVertexList->SetAttribute("Count", Vnum);

					std::wcout << L"Get count:" + ToString(Vnum) + L"\n";
					std::wcout << L"Layout count:" + ToString(Layoutnum) + L"\n";
					int Vsize = objects_info.back().VertexSize;
					//Read Layout Info
					for (int k = 0; k < Layoutnum; k++)
					{
						int newcurpos = curpos + objects_info.back().LayoutOffset + (k*0x10);

						objects_layout.push_back(ReadObjectLayout(newcurpos, buffer));

						tinyxml2::XMLElement* xmlNode = xmlLayout->InsertNewChildElement("Value");

						xmlNode->SetAttribute("type", objects_layout.back().type);
						xmlNode->SetAttribute("offset", objects_layout.back().offset);
						xmlNode->SetAttribute("channel", objects_layout.back().channel);
						xmlNode->SetAttribute("name",  objects_layout.back().name.c_str());

						xmlNode->SetAttribute("debugIndex", k);
						
					}
					//Read Vertex
					if (!onecore)
					{
						SYSTEM_INFO sysInfo;
						GetSystemInfo(&sysInfo);
						int threads_num = sysInfo.dwNumberOfProcessors;
						// Read vertices using multiple cores
						std::vector<std::thread> threads;
						threads.reserve(static_cast<size_t>(threads_num));
						int avg_num = Layoutnum / threads_num;
						// thread lock
						std::mutex mtx;
						// Multithreading seems to be the same speed as single threading
						for (int tk = 0; tk <= avg_num; tk++)
						{
							for (int th = 0; th < threads_num; th++)
							{
								int k = (tk * threads_num) + th;
								if (k < Layoutnum)
								{
									threads.emplace_back([&, k]() {
										int curoffset = objects_layout[k].offset;
										std::string curstr = objects_layout[k].name;
										//must be locked, otherwise the order will be out of order
										mtx.lock();
										tinyxml2::XMLElement* xmlVertex = xmlVertexList->InsertNewChildElement(curstr.c_str());
										mtx.unlock();

										int Vtype = objects_layout[k].type;
										xmlVertex->SetAttribute("type", Vtype);
										xmlVertex->SetAttribute("channel", objects_layout[k].channel);

										int Voffset = curpos + objects_info.back().VertexOffset + curoffset;
										std::wcout << L"vertex type:" + UTF8ToWide(curstr) + L"\n";
										//Read data
										ReadVertexMT(mtx, Voffset, buffer, Vtype, Vnum, Vsize, xmlVertex);
										//output result
										});
								}
								else
								{
									threads.emplace_back([th]() {
										std::wcout << L"no tasks are assigned to thread "+ ToString(th) + L"\n";
										});
								}
								// end
							}
							
							for (auto& t : threads) {
								t.join();
							}
							threads.clear();

							//output result
							std::wcout << L"parsing complete.\n";
						}
					}
					else
					{
						// Single core reads are more stable, but slower.
						for (int k = 0; k < Layoutnum; k++)
						{
							int curoffset = objects_layout[k].offset;
							std::string curstr = objects_layout[k].name;

							tinyxml2::XMLElement* xmlVertex = xmlVertexList->InsertNewChildElement(curstr.c_str());
							int Vtype = objects_layout[k].type;
							xmlVertex->SetAttribute("type", Vtype);
							xmlVertex->SetAttribute("channel", objects_layout[k].channel);

							int Voffset = curpos + objects_info.back().VertexOffset + curoffset;
							std::wcout << L"vertex type:" + UTF8ToWide(curstr) + L", ";
							//Read data
							ReadVertex(Voffset, buffer, Vtype, Vnum, Vsize, xmlVertex);
							//output result
							std::wcout << L"parsing complete.\n";
						}
					}
					//read the rest
					
					//Read indices
					int iNum = objects_info.back().indicesNum;
					std::wcout << L"Read indices......\n";
					std::wcout << L"Get count:" + ToString(iNum) + L"\n";

					tinyxml2::XMLElement* xmlIndices = xmlMesh->InsertNewChildElement("Indices");
					xmlIndices->SetAttribute("Count", iNum);
					for (int k = 0; k < iNum; k++)
					{
						int newcurpos = curpos + objects_info.back().indicesOffset + (k * 2);
						tinyxml2::XMLElement* xmlNode = xmlIndices->InsertNewChildElement("value");
						//short int16 = ReadInt16(buffer, newcurpos);
						char seg[2];
						seg[0] = buffer[newcurpos + 1];
						seg[1] = buffer[newcurpos];

						short int16 = 0;
						for (int i = 0; i < 2; i++)
						{
							int16 <<= 8;
							int16 |= seg[i];
						}
						xmlNode->SetText(int16);
						//xmlNode->SetText(ReadInt16(buffer, newcurpos));
					}
					std::wcout << L"complete.\n";
				}
				//mark 3
			}
			//mark 2
			std::wcout << L"Completed!\n\n";
		}
		// last get material table:
		if (MaterialCount > 0)
		{
			tinyxml2::XMLElement* xmlMatHeader = xmlHeader->InsertNewChildElement("Materials");
			xmlMatHeader->SetAttribute("count", MaterialCount);

			std::wcout << L"Getting material list...... ";

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

					// If mapping and name have different lengths
					std::wstring wstrm = textures[tempint].mapping;
					std::wstring wstrn = textures[tempint].filename;
					size_t wmsize = wstrm.size();
					size_t wnsize = wstrn.size();
					// Truncate the tail of the mapping as a mipmap
					std::string mipmap;
					if (wmsize == wnsize)
						mipmap = "0";
					else
						mipmap = WideToUTF8(wstrm.substr(wnsize, wmsize - wnsize));

					xmlMatTexID->SetAttribute("MIP", mipmap.c_str());
					utf8str = WideToUTF8(wstrn);
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
			std::wcout << L"Completed!\n\n";
		}
		// Read End!
		
		// Write to file
		std::string outfile = WideToUTF8(path) + "_MDB.xml";
		xml.SaveFile(outfile.c_str());
		/*
		tinyxml2::XMLPrinter printer;
		xml.Accept(&printer);
		auto xmlString = std::string{ printer.CStr() };

		std::wcout << UTF8ToWide(xmlString);
		FindAndReplaceAll(xmlString, "\n", "\r\n");
		
		std::ofstream output(path + L"_MDB.xml", std::ios::binary | std::ios::out | std::ios::ate);
		
		std::locale utf8_locale(output.getloc(), new std::codecvt_utf8<wchar_t>);
		output.imbue(utf8_locale);
		output << xmlString;
		output.close();
		*/
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
	// this is index
	int position = pos;
	Read4Bytes(seg, buffer, position);
	out.index[0] = GetIntFromChunk(seg);
	// this is parent
	position = pos + 0x4;
	Read4Bytes(seg, buffer, position);
	out.index[1] = GetIntFromChunk(seg);

	position = pos + 0x8;
	Read4Bytes(seg, buffer, position);
	out.index[2] = GetIntFromChunk(seg);
	position = pos + 0x0C;
	Read4Bytes(seg, buffer, position);
	out.index[3] = GetIntFromChunk(seg);
	// this may be index of bone name
	position = pos + 0x10;
	Read4Bytes(seg, buffer, position);
	out.index[4] = GetIntFromChunk(seg);

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

void CMDBtoXML::ReadVertex(int pos, std::vector<char> buffer, int type, int num, int size, tinyxml2::XMLElement* header)
{
	if (type == 1)
	{
		float vf[4];

		for (int l = 0; l < num; l++)
		{
			int Vcurpos = pos + (l * size);

			memcpy(&vf, &buffer[Vcurpos], 16U);

			tinyxml2::XMLElement* xmlVNode = header->InsertNewChildElement("V");
			xmlVNode->SetAttribute("x", vf[0]);
			xmlVNode->SetAttribute("y", vf[1]);
			xmlVNode->SetAttribute("z", vf[2]);
			xmlVNode->SetAttribute("w", vf[3]);
		}
	}
	else if (type == 4)
	{
		float vf[3];

		for (int l = 0; l < num; l++)
		{
			int Vcurpos = pos + (l * size);

			memcpy(&vf, &buffer[Vcurpos], 12U);

			tinyxml2::XMLElement* xmlVNode = header->InsertNewChildElement("V");
			xmlVNode->SetAttribute("x", vf[0]);
			xmlVNode->SetAttribute("y", vf[1]);
			xmlVNode->SetAttribute("z", vf[2]);
		}
	}
	else if (type == 7)
	{
		half_float::half vf[4];

		for (int l = 0; l < num; l++)
		{
			int Vcurpos = pos + (l * size);

			memcpy(&vf, &buffer[Vcurpos], 8U);

			tinyxml2::XMLElement* xmlVNode = header->InsertNewChildElement("V");
			xmlVNode->SetAttribute("x", vf[0]);
			xmlVNode->SetAttribute("y", vf[1]);
			xmlVNode->SetAttribute("z", vf[2]);
			xmlVNode->SetAttribute("w", vf[3]);
		}
	}
	else if (type == 12)
	{
		float vf[2];

		for (int l = 0; l < num; l++)
		{
			int Vcurpos = pos + (l * size);

			memcpy(&vf, &buffer[Vcurpos], 8U);

			tinyxml2::XMLElement* xmlVNode = header->InsertNewChildElement("V");
			xmlVNode->SetAttribute("x", vf[0]);
			xmlVNode->SetAttribute("y", vf[1]);
		}
	}
	else if (type == 21)
	{
		unsigned char seg[4];
		for (int l = 0; l < num; l++)
		{
			int Vcurpos = pos + (l * size);

			Read4BytesReversed(seg, buffer, Vcurpos);

			tinyxml2::XMLElement* xmlVNode = header->InsertNewChildElement("V");
			xmlVNode->SetAttribute("x", seg[0]);
			xmlVNode->SetAttribute("y", seg[1]);
			xmlVNode->SetAttribute("z", seg[2]);
			xmlVNode->SetAttribute("w", seg[3]);
		}
	}
	else
	{
		// unknown type
		std::string strn = ReadRaw(buffer, pos, 0x20);
		header->SetText(strn.c_str());
	}
}

void CMDBtoXML::ReadVertexMT(std::mutex& mtx, int pos, std::vector<char> buffer, int type, int num, int size, tinyxml2::XMLElement* header)
{
	if (type == 1)
	{
		struct ModelVertex
		{
			float vf[4];
		};

		std::vector <ModelVertex> vf(num);

		for (int l = 0; l < num; l++)
		{
			int Vcurpos = pos + (l * size);

			memcpy(&vf[l], &buffer[Vcurpos], 16U);

			//Locking, it seems to become a single core, but does not affect the efficiency?
			std::lock_guard<std::mutex> lk(mtx);
			tinyxml2::XMLElement* xmlVNode = header->InsertNewChildElement("V");
			xmlVNode->SetAttribute("x", vf[l].vf[0]);
			xmlVNode->SetAttribute("y", vf[l].vf[1]);
			xmlVNode->SetAttribute("z", vf[l].vf[2]);
			xmlVNode->SetAttribute("w", vf[l].vf[3]);
		}
	}
	else if (type == 4)
	{
		struct ModelVertex
		{
			float vf[3];
		};

		std::vector <ModelVertex> vf(num);

		for (int l = 0; l < num; l++)
		{
			int Vcurpos = pos + (l * size);

			memcpy(&vf[l], &buffer[Vcurpos], 12U);

			std::lock_guard<std::mutex> lk(mtx);
			tinyxml2::XMLElement* xmlVNode = header->InsertNewChildElement("V");
			xmlVNode->SetAttribute("x", vf[l].vf[0]);
			xmlVNode->SetAttribute("y", vf[l].vf[1]);
			xmlVNode->SetAttribute("z", vf[l].vf[2]);
		}
	}
	else if (type == 7)
	{
		struct ModelVertex
		{
			half_float::half vf[4];
		};

		std::vector <ModelVertex> vf(num);

		for (int l = 0; l < num; l++)
		{
			int Vcurpos = pos + (l * size);

			memcpy(&vf[l], &buffer[Vcurpos], 8U);

			std::lock_guard<std::mutex> lk(mtx);
			tinyxml2::XMLElement* xmlVNode = header->InsertNewChildElement("V");
			xmlVNode->SetAttribute("x", vf[l].vf[0]);
			xmlVNode->SetAttribute("y", vf[l].vf[1]);
			xmlVNode->SetAttribute("z", vf[l].vf[2]);
			xmlVNode->SetAttribute("w", vf[l].vf[3]);
		}
	}
	else if (type == 12)
	{
		struct ModelVertex
		{
			float vf[2];
		};

		std::vector <ModelVertex> vf(num);

		for (int l = 0; l < num; l++)
		{
			int Vcurpos = pos + (l * size);

			memcpy(&vf[l], &buffer[Vcurpos], 8U);

			std::lock_guard<std::mutex> lk(mtx);
			tinyxml2::XMLElement* xmlVNode = header->InsertNewChildElement("V");
			xmlVNode->SetAttribute("x", vf[l].vf[0]);
			xmlVNode->SetAttribute("y", vf[l].vf[1]);
		}
	}
	else if (type == 21)
	{
		struct ModelVertex
		{
			unsigned char vf[4];
		};

		std::vector <ModelVertex> vf(num);

		for (int l = 0; l < num; l++)
		{
			int Vcurpos = pos + (l * size);

			memcpy(&vf[l], &buffer[Vcurpos], 4U);

			std::lock_guard<std::mutex> lk(mtx);
			tinyxml2::XMLElement* xmlVNode = header->InsertNewChildElement("V");
			xmlVNode->SetAttribute("x", vf[l].vf[0]);
			xmlVNode->SetAttribute("y", vf[l].vf[1]);
			xmlVNode->SetAttribute("z", vf[l].vf[2]);
			xmlVNode->SetAttribute("w", vf[l].vf[3]);
			/*
			unsigned char seg[4];
			Read4BytesReversed(seg, buffer, Vcurpos);
			xmlVNode->SetAttribute("x", seg[0]);
			xmlVNode->SetAttribute("y", seg[1]);
			xmlVNode->SetAttribute("z", seg[2]);
			xmlVNode->SetAttribute("w", seg[3]);
			*/
		}
	}
	else
	{
		// unknown type
		std::string strn = ReadRaw(buffer, pos, 0x20);

		std::lock_guard<std::mutex> lk(mtx);
		header->SetText(strn.c_str());
	}
}

#include <filesystem>

void CXMLToMDB::Write(const std::wstring& path, bool multcore)
{
	std::wstring sourcePath = path + L"_mdb.xml";
	//std::wstring FileRaw = ReadFile(sourcePath.c_str());
	std::string UTF8Path = WideToUTF8(sourcePath);

	//Set to header size.
	std::vector< char > bytes(0x30);
	//Write header.
	GenerateHeader(bytes);

	tinyxml2::XMLDocument doc;
	doc.LoadFile(UTF8Path.c_str());

	tinyxml2::XMLNode* header = doc.FirstChildElement("MDB");
	tinyxml2::XMLElement* entry, * entry2, * entry3, * entry4, * entry5, * entry6;

	// read model name table (if there is)
	bool NoNameTable = false;
	entry = header->FirstChildElement("Names");
	if (entry == nullptr)
	{
		NoNameTable = true;
		std::wcout << L"Name table does not exist.\n";
	}
	else
	{
		std::wcout << L"Name table exists.\nLoad name table:\n";
		for (entry2 = entry->FirstChildElement("value"); entry2 != 0; entry2 = entry2->NextSiblingElement("value"))
		{
			std::string tempName = entry2->GetText();
			std::wstring modelName = UTF8ToWide(tempName);

			m_vecWNames.push_back(modelName);
			WriteWStringToTemp(modelName);

			NameCount++;

			std::wcout << modelName + L", ";
		}
		std::wcout << L"\nLoading completed!\n";
	}
	// read texture table (if there is)
	bool NoTexTable = false;
	entry = header->FirstChildElement("Textures");
	if (entry == nullptr)
	{
		NoTexTable = true;
		std::wcout << L"Texture table does not exist.\n";
	}
	else
	{
		std::wcout << L"Texture table exists. Use texture table...\n";
		for (entry2 = entry->FirstChildElement("value"); entry2 != 0; entry2 = entry2->NextSiblingElement("value"))
		{
			m_vecTexture.push_back( GetTexture(entry2) );
		}
	}
	// read bone list (it must exist)
	entry = header->FirstChildElement("BoneLists");
	if (entry == nullptr)
	{
		std::wcout << L"Required \'BoneLists\' do not exist!\n";
		//terminate program
		system("pause");
		exit(0);
	}
	else
	{
		std::wcout << L"Converting BoneLists:\n";
		for (entry2 = header->FirstChildElement(); entry2 != 0; entry2 = entry2->NextSiblingElement("Bone"))
		{
		}
	}

	/*
	for (entry = header->FirstChildElement(); entry != 0; entry = entry->NextSiblingElement())
	{
	}
	*/

	//Push name table count
	Set4BytesInFile(bytes, 0x8, NameTableCount);
	//Push name table
	for (int i = 0; i < NameTableCount; i++)
	{
		if(i < NameCount)
			m_vecNameTpos.push_back(bytes.size());

		for (int j = 0; j < 4; j++)
			bytes.push_back(0);
	}

	//Push texture table count
	Set4BytesInFile(bytes, 0x28, TextureCount);
	//Push texture table offset
	Set4BytesInFile(bytes, 0x2C, bytes.size());
	//Push texture table
	for (size_t i = 0; i < m_vecTexture.size(); i++)
	{
		m_vecTexPos.push_back(bytes.size());
		for (size_t j = 0; j < m_vecTexture[i].bytes.size(); j++)
		{
			bytes.push_back(m_vecTexture[i].bytes[j]);
		}
	}
	
	//Push wide strings
	for (size_t i = 0; i < m_vecWStrns.size(); i++)
	{
		int strpos = bytes.size();
		PushWStringToVector(m_vecWStrns[i], &bytes);
		//write string offset
		//in name table:
		for (size_t j = 0; j < m_vecNameTpos.size(); j++)
		{
			if (m_vecWNames[j] == m_vecWStrns[i])
				Set4BytesInFile(bytes, m_vecNameTpos[j], strpos - m_vecNameTpos[j]);
		}
		//in texture table:
		for (int j = 0; j < m_vecTexPos.size(); j++)
		{
			if (m_vecTexture[j].mapping == m_vecWStrns[i])
				Set4BytesInFile(bytes, m_vecTexPos[j] + 0x4, strpos - m_vecTexPos[j]);
			if (m_vecTexture[j].filename == m_vecWStrns[i])
				Set4BytesInFile(bytes, m_vecTexPos[j] + 0x8, strpos - m_vecTexPos[j]);
		}
	}

	
	//Final write.
	/*
	std::ofstream newFile(path + L".mdb", std::ios::binary | std::ios::out | std::ios::ate);
	
	for (int i = 0; i < bytes.size(); i++)
	{
		newFile << bytes[i];
	}
	
	newFile.close();
	*/
	std::wcout << L"Conversion completed: " + sourcePath + L"\n";
}

void CXMLToMDB::Set4BytesInFile(std::vector<char>& bytes, int pos, int value)
{
	char* fnofs = IntToBytes(value);

	bytes[pos] = fnofs[0];
	bytes[pos+1] = fnofs[1];
	bytes[pos+2] = fnofs[2];
	bytes[pos+3] = fnofs[3];

	free(fnofs);
}

void CXMLToMDB::GenerateHeader(std::vector< char >& bytes)
{
	//Header "MDB0"
	bytes[0] = 0x4D;
	bytes[1] = 0x44;
	bytes[2] = 0x42;
	bytes[3] = 0x30;
	//0x14 00 00 00
	bytes[4] = 0x14;
	//NameTable starts at 0x30
	bytes[0xC] = 0x30;
}

MDBTexture CXMLToMDB::GetTexture(tinyxml2::XMLElement* entry2)
{
	MDBTexture out;
	std::string tempName;

	out.ID = TextureCount;

	tempName = entry2->Attribute("mapping");
	out.mapping = UTF8ToWide(tempName);

	tempName = entry2->Attribute("filename");
	out.filename = UTF8ToWide(tempName);

	out.bytes.resize(0x10);
	// write index
	char* idBytes = IntToBytes(out.ID);

	out.bytes[0] = idBytes[0];
	out.bytes[1] = idBytes[1];
	out.bytes[2] = idBytes[2];
	out.bytes[3] = idBytes[3];

	free(idBytes);
	// write name
	std::wstring wstr;

	wstr = out.mapping;
	WriteWStringToTemp(wstr);
	wstr = out.filename;
	WriteWStringToTemp(wstr);
	// increment count
	TextureCount++;

	//std::wcout << mapping + L", " + filename + L"\n";
	return out;
}

void CXMLToMDB::WriteWStringToTemp(std::wstring wstr)
{
	//Check string array:
	bool found = false;
	for (size_t strID = 0; strID < m_vecWStrns.size(); strID++)
	{
		if (m_vecWStrns[strID] == wstr)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		m_vecWStrns.push_back(wstr);
	}

	NameTableCount++;
}