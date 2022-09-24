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
				xmlBoneP->SetAttribute("value", bones.back().index[1]);
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
				//Read matrix1
				for (int j = 0; j < 4; j++)
				{
					tpos = curtablepos + 0x20 + (j * 0x10);

					tinyxml2::XMLElement* xmlBNode = xmlBone->InsertNewChildElement("mainTM");
					float bf[4];

					memcpy(&bf, &buffer[tpos], 16U);
					xmlBNode->SetAttribute("x", bf[0]);
					xmlBNode->SetAttribute("y", bf[1]);
					xmlBNode->SetAttribute("z", bf[2]);
					xmlBNode->SetAttribute("w", bf[3]);
				}
				//Read matrix2
				for (int j = 0; j < 4; j++)
				{
					tpos = curtablepos + 0x60 + (j * 0x10);

					tinyxml2::XMLElement* xmlBNode = xmlBone->InsertNewChildElement("skinTM");
					float bf[4];

					memcpy(&bf, &buffer[tpos], 16U);
					xmlBNode->SetAttribute("x", bf[0]);
					xmlBNode->SetAttribute("y", bf[1]);
					xmlBNode->SetAttribute("z", bf[2]);
					xmlBNode->SetAttribute("w", bf[3]);
				}
				//Read Position
				{
					tpos = curtablepos + 0xA0;

					tinyxml2::XMLElement* xmlBNode = xmlBone->InsertNewChildElement("position");
					float bf[4];

					memcpy(&bf, &buffer[tpos], 16U);
					xmlBNode->SetAttribute("x", bf[0]);
					xmlBNode->SetAttribute("y", bf[1]);
					xmlBNode->SetAttribute("z", bf[2]);
					xmlBNode->SetAttribute("w", bf[3]);
				}
				//Read Float
				{
					tpos = curtablepos + 0xB0;

					tinyxml2::XMLElement* xmlBNode = xmlBone->InsertNewChildElement("float");
					float bf[4];

					memcpy(&bf, &buffer[tpos], 16U);
					xmlBNode->SetAttribute("x", bf[0]);
					xmlBNode->SetAttribute("y", bf[1]);
					xmlBNode->SetAttribute("z", bf[2]);
					xmlBNode->SetAttribute("w", bf[3]);

					utf8str = ReadRaw(buffer, tpos, 0x10);
					xmlBNode->SetText(utf8str.c_str());
					xmlBNode->SetAttribute("debugPos", tpos);
				}
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
					//xmlLayout->SetAttribute("inpos", objects_info.back().LayoutOffset);

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

										int Vtype = objects_layout[k].type;
										xmlVertex->SetAttribute("type", Vtype);
										xmlVertex->SetAttribute("channel", objects_layout[k].channel);

										int Voffset = curpos + objects_info.back().VertexOffset + curoffset;
										xmlVertex->InsertNewChildElement("debug")->SetAttribute("pos", Voffset);
										mtx.unlock();

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
							xmlVertex->InsertNewChildElement("debug")->SetAttribute("pos", Voffset);

							std::wcout << L"vertex type:" + UTF8ToWide(curstr) + L", ";
							//Read data
							ReadVertex(Voffset, buffer, Vtype, Vnum, Vsize, xmlVertex);
							//output result
							std::wcout << L"parsing complete.\n";
						}
					}
					//Clear temp
					objects_layout.clear();
					
					//Read faces
					int iNum = objects_info.back().indicesNum;
					std::wcout << L"Read faces......\n";
					std::wcout << L"Get count:" + ToString(iNum) + L"\n";
					//Unify with the name in 3dmax
					tinyxml2::XMLElement* xmlIndices = xmlMesh->InsertNewChildElement("Faces");
					xmlIndices->SetAttribute("Count", iNum);

					short int16;
					for (int k = 0; k < iNum; k++)
					{
						int newcurpos = curpos + objects_info.back().indicesOffset + (k * 2);
						tinyxml2::XMLElement* xmlNode = xmlIndices->InsertNewChildElement("value");
						/* wrong reading
						char seg[2];
						seg[0] = buffer[newcurpos + 1];
						seg[1] = buffer[newcurpos];

						short int16 = 0;
						for (int i = 0; i < 2; i++)
						{
							int16 <<= 8;
							int16 |= seg[i];
						}
						*/
						memcpy(&int16, &buffer[newcurpos], 2U);
						//xmlNode->SetAttribute("pos", newcurpos);
						xmlNode->SetAttribute("value", int16);
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
				xmlMatSdr->SetAttribute("ptrnum", materials.back().PtrCount);
				xmlMatSdr->SetAttribute("texnum", materials.back().TexCount);
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

					//utf8str = std::to_string(materials_ptr.back().r) + ", ";
					//utf8str += std::to_string(materials_ptr.back().g) + ", ";
					//utf8str += std::to_string(materials_ptr.back().b) + ", ";
					//utf8str += std::to_string(materials_ptr.back().a);
					tinyxml2::XMLElement* xmlMatPtrVal = xmlMatPtr->InsertNewChildElement("Color");
					xmlMatPtrVal->SetAttribute("r", materials_ptr.back().r);
					xmlMatPtrVal->SetAttribute("g", materials_ptr.back().g);
					xmlMatPtrVal->SetAttribute("b", materials_ptr.back().b);
					xmlMatPtrVal->SetAttribute("a", materials_ptr.back().a);

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
					//check mapping length
					size_t nnsize = wstrm.find_last_of(L"_") + 4;
					size_t wmsize = wstrm.size();
					// Truncate the tail of the mapping as a mipmap
					std::string mipmap;
					if (wmsize == nnsize)
						mipmap = "0";
					else
						mipmap = WideToUTF8(wstrm.substr(nnsize, wmsize - nnsize));
					//Now it has a problem
					//so do not use it
					/*
					size_t wnsize = wstrn.size();
					if (wmsize == wnsize)
						mipmap = "0";
					else
						mipmap = WideToUTF8(wstrm.substr(wnsize, wmsize - wnsize));
					*/

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
	//Read4BytesReversed(seg, buffer, position);
	//memcpy(&f, &seg, sizeof(f));
	memcpy(&f, &buffer[position], 4U);
	out.r = f;

	position = pos + 0x4;
	memcpy(&f, &buffer[position], 4U);
	out.g = f;

	position = pos + 0x8;
	memcpy(&f, &buffer[position], 4U);
	out.b = f;

	position = pos + 0x0C;
	memcpy(&f, &buffer[position], 4U);
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
	short int16;

	int position = pos + 0x4;
	Read4Bytes(seg, buffer, position);
	out.matid = GetIntFromChunk(seg);

	position = pos + 0x0C;
	Read4Bytes(seg, buffer, position);
	out.LayoutOffset = GetIntFromChunk(seg);

	position = pos + 0x10;
	memcpy(&int16, &buffer[position], 2U);
	out.VertexSize = int16;
	//out.VertexSize = ReadInt16(buffer, position);
	position = pos + 0x12;
	memcpy(&int16, &buffer[position], 2U);
	out.LayoutCount = int16;

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
			//The performance of this function is too poor
			//Read4BytesReversed(seg, buffer, Vcurpos);
			memcpy(&seg, &buffer[Vcurpos], 4U);

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

	tinyxml2::XMLDocument doc;
	doc.LoadFile(UTF8Path.c_str());

	tinyxml2::XMLNode* header = doc.FirstChildElement("MDB");
	tinyxml2::XMLElement* entry, * entry2;

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
		std::wcout << L"\nLoading completed!\n\n";
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
		std::wcout << L"Loading completed!\n\n";
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
		for (entry2 = entry->FirstChildElement(); entry2 != 0; entry2 = entry2->NextSiblingElement("Bone"))
		{
			m_vecBone.push_back( GetBone(entry2, NoNameTable) );
		}
		std::wcout << L"-> bone count: " + ToString(BoneCount) + L"\n\n";
	}
	// read model list (it must exist)
	entry = header->FirstChildElement("ObjectLists");
	if (entry == nullptr)
	{
		std::wcout << L"Required \'ObjectLists\' do not exist!\n";
		//terminate program
		system("pause");
		exit(0);
	}
	else
	{
		std::wcout << L"Converting objects:\n";
		for (entry2 = entry->FirstChildElement(); entry2 != 0; entry2 = entry2->NextSiblingElement("Object"))
		{
			std::wcout << L"read model:\n";
			m_vecObject.push_back( GetModel(entry2, NoNameTable, multcore) );
			std::wcout << L"write model complete!\n\n";
		}
		std::wcout << L"-> object count: " + ToString(ObjectCount) + L"\n\n";
	}
	// read material list (it must exist)
	entry = header->FirstChildElement("Materials");
	if (entry == nullptr)
	{
		std::wcout << L"Required \'Materials\' do not exist!\n";
		//terminate program
		system("pause");
		exit(0);
	}
	else
	{
		std::wcout << L"Converting Materials:\n";
		for (entry2 = entry->FirstChildElement(); entry2 != 0; entry2 = entry2->NextSiblingElement("MaterialNode"))
		{
			m_vecMaterial.push_back( GetMaterial(entry2, NoNameTable, NoTexTable) );
		}
		std::wcout << L"-> material count: " + ToString(MaterialCount) + L"\n\n";
	}

	//Set to header size.
	std::vector< char > bytes(0x30);
	//Write header.
	GenerateHeader(bytes);

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

	//Push bone list count
	Set4BytesInFile(bytes, 0x10, BoneCount);
	//Push bone list offset
	Set4BytesInFile(bytes, 0x14, bytes.size());
	//Push bone list
	for (size_t i = 0; i < m_vecBone.size(); i++)
	{
		for (size_t j = 0; j < m_vecBone[i].bytes.size(); j++)
		{
			bytes.push_back(m_vecBone[i].bytes[j]);
		}
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

	//Push material list count
	Set4BytesInFile(bytes, 0x20, MaterialCount);
	//Push material list offset
	Set4BytesInFile(bytes, 0x24, bytes.size());
	//Push material list
	for (size_t i = 0; i < m_vecMaterial.size(); i++)
	{
		m_vecMatPos.push_back(bytes.size());
		for (size_t j = 0; j < m_vecMaterial[i].bytes.size(); j++)
		{
			bytes.push_back(m_vecMaterial[i].bytes[j]);
		}
	}
	//Push material list parameter
	for (size_t i = 0; i < m_vecMaterialPtr.size(); i++)
	{
		m_vecMatPtrPos.push_back(bytes.size());
		for (size_t j = 0; j < m_vecMaterialPtr[i].bytes.size(); j++)
		{
			bytes.push_back(m_vecMaterialPtr[i].bytes[j]);
		}
	}
	//Push material list texture
	for (size_t i = 0; i < m_vecMaterialTex.size(); i++)
	{
		m_vecMatTexPos.push_back(bytes.size());
		for (size_t j = 0; j < m_vecMaterialTex[i].bytes.size(); j++)
		{
			bytes.push_back(m_vecMaterialTex[i].bytes[j]);
		}
	}
	//Actually a material's parameters and texture are together,
	//Now output parameters and textures separately.
	//Write parameter offset to material
	int matptrNum = 0;
	int mattexNum = 0;
	for (size_t i = 0; i < m_vecMatPos.size(); i++)
	{
		Set4BytesInFile(bytes, (m_vecMatPos[i] + 0x0C), (m_vecMatPtrPos[matptrNum] - m_vecMatPos[i]));
		matptrNum += m_vecMaterial[i].PtrCount;
		Set4BytesInFile(bytes, (m_vecMatPos[i] + 0x14), (m_vecMatTexPos[mattexNum] - m_vecMatPos[i]));
		mattexNum += m_vecMaterial[i].TexCount;
	}

	//Push model list count
	Set4BytesInFile(bytes, 0x18, ObjectCount);
	//Push model list offset
	Set4BytesInFile(bytes, 0x1C, bytes.size());
	//Push model list
	for (size_t i = 0; i < m_vecObject.size(); i++)
	{
		m_vecModelPos.push_back(bytes.size());
		for (size_t j = 0; j < m_vecObject[i].bytes.size(); j++)
		{
			bytes.push_back(m_vecObject[i].bytes[j]);
		}
	}
	//Push model info
	for (size_t i = 0; i < m_vecObjInfo.size(); i++)
	{
		m_vecObjInfoPos.push_back(bytes.size());
		for (size_t j = 0; j < m_vecObjInfo[i].bytes.size(); j++)
		{
			bytes.push_back(m_vecObjInfo[i].bytes[j]);
		}
	}
	//Push model layout
	for (size_t i = 0; i < m_vecObjLayout.size(); i++)
	{
		m_vecObjLayPos.push_back(bytes.size());
		for (size_t j = 0; j < m_vecObjLayout[i].bytes.size(); j++)
		{
			bytes.push_back(m_vecObjLayout[i].bytes[j]);
		}
		//Data tails for each model may need to be aligned
		//AlignFileTo16Bytes(bytes);
	}
	//Push model indices
	for (size_t i = 0; i < m_vecObjIndices.size(); i++)
	{
		// write offset to model info
		Set4BytesInFile(bytes, (m_vecObjInfoPos[i] + 0x24), (bytes.size() - m_vecObjInfoPos[i]));
		for (size_t j = 0; j < m_vecObjIndices[i].bytes.size(); j++)
		{
			bytes.push_back(m_vecObjIndices[i].bytes[j]);
		}
		//AlignFileTo16Bytes(bytes);
	}
	//Push model vertices
	for (size_t i = 0; i < m_vecObjVertices.size(); i++)
	{
		// write offset to model info
		Set4BytesInFile(bytes, (m_vecObjInfoPos[i] + 0x1C), (bytes.size() - m_vecObjInfoPos[i]));
		for (size_t j = 0; j < m_vecObjVertices[i].bytes.size(); j++)
		{
			bytes.push_back(m_vecObjVertices[i].bytes[j]);
		}
		// The last one does not need to be aligned
		/*
		if((i+1) < m_vecObjVertices.size())
			AlignFileTo16Bytes(bytes);
		*/
	}
	//Write parameter offset to model
	int modelInfoNum = 0;
	for (size_t i = 0; i < m_vecModelPos.size(); i++)
	{
		Set4BytesInFile(bytes, (m_vecModelPos[i] + 0x0C), (m_vecObjInfoPos[modelInfoNum] - m_vecModelPos[i]));
		modelInfoNum += m_vecObject[i].infoCount;
	}
	//Write parameter offset to model info
	int MIlayoutNum = 0;
	for (size_t i = 0; i < m_vecObjInfoPos.size(); i++)
	{
		Set4BytesInFile(bytes, (m_vecObjInfoPos[i] + 0x0C), (m_vecObjLayPos[MIlayoutNum] - m_vecObjInfoPos[i]));
		MIlayoutNum += m_vecObjInfo[i].LayoutCount;
	}

	//Push strings
	for (size_t i = 0; i < m_vecStrns.size(); i++)
	{
		int strpos = bytes.size();
		PushStringToVector(m_vecStrns[i], &bytes);
		//write string offset
		//in material list:
		for (size_t j = 0; j < m_vecMatPtrPos.size(); j++)
		{
			if (m_vecMaterialPtr[j].ptrname == m_vecStrns[i])
				Set4BytesInFile(bytes, (m_vecMatPtrPos[j] + 0x18), (strpos - m_vecMatPtrPos[j]) );
		}
		for (size_t j = 0; j < m_vecMatTexPos.size(); j++)
		{
			if (m_vecMaterialTex[j].textype == m_vecStrns[i])
				Set4BytesInFile(bytes, (m_vecMatTexPos[j] + 0x4), (strpos - m_vecMatTexPos[j]) );
		}
		//in model list
		for (size_t j = 0; j < m_vecObjLayPos.size(); j++)
		{
			if (m_vecObjLayout[j].name == m_vecStrns[i])
				Set4BytesInFile(bytes, (m_vecObjLayPos[j] + 0xC), (strpos - m_vecObjLayPos[j]) );
		}
	}
	bytes.push_back(0);
	
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
				Set4BytesInFile(bytes, m_vecNameTpos[j], (strpos - m_vecNameTpos[j]) );
		}
		//in texture table:
		for (size_t j = 0; j < m_vecTexPos.size(); j++)
		{
			if (m_vecTexture[j].mapping == m_vecWStrns[i])
				Set4BytesInFile(bytes, (m_vecTexPos[j] + 0x4), (strpos - m_vecTexPos[j]) );
			if (m_vecTexture[j].filename == m_vecWStrns[i])
				Set4BytesInFile(bytes, (m_vecTexPos[j] + 0x8), (strpos - m_vecTexPos[j]) );
		}
		//in material list:
		for (size_t j = 0; j < m_vecMatPos.size(); j++)
		{
			if (m_vecMaterial[j].shader == m_vecWStrns[i])
				Set4BytesInFile(bytes, (m_vecMatPos[j] + 0x8), (strpos - m_vecMatPos[j]) );
		}
	}

	std::wcout << L">> File Size: " + ToString((int)bytes.size()) + L" Bytes!\n";
	//Final write.
	/**/
	std::ofstream newFile(path + L".mdb", std::ios::binary | std::ios::out | std::ios::ate);
	
	for (int i = 0; i < bytes.size(); i++)
	{
		newFile << bytes[i];
	}
	
	newFile.close();
	
	std::wcout << L"Conversion completed: " + sourcePath + L"\n";
}

void CXMLToMDB::AlignFileTo16Bytes(std::vector<char>& bytes)
{
	//align size to 16 bytes
	int filesize = bytes.size() % 16;
	if (filesize > 0)
	{
		for (int i = filesize; i < 16; i++)
			bytes.push_back(0);
	}
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

MDBBone CXMLToMDB::GetBone(tinyxml2::XMLElement* entry2, bool NoNameTable)
{
	MDBBone out;
	tinyxml2::XMLElement* entry3;

	entry3 = entry2->FirstChildElement("name");
	int nameid = entry3->IntAttribute("id");
	if (!NoNameTable && nameid)
	{
		out.index[0] = nameid;
	}
	else
	{
		std::wstring wstr = UTF8ToWide(entry3->GetText());
		// Check if name exists in table
		bool found = false;
		for (size_t strID = 0; strID < m_vecWNames.size(); strID++)
		{
			if (m_vecWNames[strID] == wstr)
			{
				found = true;
				out.index[0] = strID;
				break;
			}
		}
		if (!found)
		{
			out.index[0] = m_vecWNames.size();
			m_vecWNames.push_back(wstr);
			NameCount++;
		}
		WriteWStringToTemp(wstr);
	}

	out.index[1] = entry2->FirstChildElement("parent")->IntAttribute("value");

	entry3 = entry2->FirstChildElement("IK");
	out.index[2] = entry3->IntAttribute("root");
	out.index[3] = entry3->IntAttribute("next");
	out.index[4] = entry3->IntAttribute("current");

	out.bytes.resize(0xC0);
	memcpy(&out.bytes[0], &out.index[0], 20U);
	// ubyte 4x3
	for (int i = 0; i < 3; i++)
	{
		entry3 = entry3->NextSiblingElement();
		out.weight[i][0] = entry3->IntAttribute("x");
		out.weight[i][1] = entry3->IntAttribute("y");
		out.weight[i][2] = entry3->IntAttribute("z");
		out.weight[i][3] = entry3->IntAttribute("w");
	}
	memcpy(&out.bytes[0x14], &out.weight[0][0], 12U);
	// matrix1 4x4
	for (int i = 0; i < 4; i++)
	{
		entry3 = entry3->NextSiblingElement();
		out.matrix1[i][0] = entry3->FloatAttribute("x");
		out.matrix1[i][1] = entry3->FloatAttribute("y");
		out.matrix1[i][2] = entry3->FloatAttribute("z");
		out.matrix1[i][3] = entry3->FloatAttribute("w");
	}
	memcpy(&out.bytes[0x20], &out.matrix1[0][0], 64U);
	// matrix2 4x4
	for (int i = 0; i < 4; i++)
	{
		entry3 = entry3->NextSiblingElement();
		out.matrix2[i][0] = entry3->FloatAttribute("x");
		out.matrix2[i][1] = entry3->FloatAttribute("y");
		out.matrix2[i][2] = entry3->FloatAttribute("z");
		out.matrix2[i][3] = entry3->FloatAttribute("w");
	}
	memcpy(&out.bytes[0x60], &out.matrix2[0][0], 64U);
	// fg 2x4
	for (int i = 0; i < 2; i++)
	{
		entry3 = entry3->NextSiblingElement();
		out.fg[i][0] = entry3->FloatAttribute("x");
		out.fg[i][1] = entry3->FloatAttribute("y");
		out.fg[i][2] = entry3->FloatAttribute("z");
		out.fg[i][3] = entry3->FloatAttribute("w");
	}
	memcpy(&out.bytes[0xA0], &out.fg[0][0], 32U);

	BoneCount++;

	return out;
}

void CXMLToMDB::WriteRawToByte(std::string& argsStrn, std::vector<char> &buf, int pos)
{
	if (argsStrn.length() % 2 > 0)
	{
		argsStrn = ("0" + argsStrn);
	}
	int count = 0;
	//Convert to hex.
	for (unsigned int i = 0; i < argsStrn.length(); i += 2)
	{
		std::string byteString = argsStrn.substr(i, 2);
		buf[pos + count] = (char)std::stol(byteString.c_str(), NULL, 16);
		count++;
	}
}

MDBMaterial CXMLToMDB::GetMaterial(tinyxml2::XMLElement* entry2, bool NoNameTable, bool NoTexTable)
{
	MDBMaterial out;
	tinyxml2::XMLElement* entry3, *entry4;
	std::wstring wstr;
	std::string argsStrn;

	out.PtrCount = 0;
	out.TexCount = 0;

	out.bytes.resize(0x20);

	entry3 = entry2->FirstChildElement("raw");
	argsStrn = entry3->GetText();
	WriteRawToByte(argsStrn, out.bytes, 0);
	// read material name
	entry3 = entry2->FirstChildElement("MaterialName");
	int nameid = entry3->IntAttribute("MatID");
	if (!NoNameTable && nameid)
	{
		out.matid = nameid;
	}
	else
	{
		std::wstring wstr = UTF8ToWide(entry3->GetText());
		// Check if name exists in table
		bool found = false;
		for (size_t strID = 0; strID < m_vecWNames.size(); strID++)
		{
			if (m_vecWNames[strID] == wstr)
			{
				found = true;
				out.matid = strID;
				break;
			}
		}
		if (!found)
		{
			out.matid = m_vecWNames.size();
			m_vecWNames.push_back(wstr);
			NameCount++;
		}
		WriteWStringToTemp(wstr);
		wstr.clear();
	}
	memcpy(&out.bytes[4], &out.matid, 4U);
	//read shader name
	//note! it is not stored in the name table!
	entry3 = entry2->FirstChildElement("Shader");
	wstr = UTF8ToWide(entry3->Attribute("Name"));
	WriteWStringToTemp(wstr);
	out.shader = wstr;
	wstr.clear();
	// read parameter
	out.PtrCount = 0;
	for (entry4 = entry3->FirstChildElement("Parameter"); entry4 != 0; entry4 = entry4->NextSiblingElement("Parameter"))
	{
		m_vecMaterialPtr.push_back(GetMaterialParameter(entry4));
		out.PtrCount++;
	}
	memcpy(&out.bytes[0x10], &out.PtrCount, 4U);
	// read texture
	out.TexCount = 0;
	for (entry4 = entry3->FirstChildElement("Texture"); entry4 != 0; entry4 = entry4->NextSiblingElement("Texture"))
	{
		m_vecMaterialTex.push_back(GetMaterialTexture(entry4, NoTexTable));
		out.TexCount++;
	}
	memcpy(&out.bytes[0x18], &out.TexCount, 4U);

	entry3 = entry3->NextSiblingElement("raw");
	argsStrn = entry3->GetText();
	WriteRawToByte(argsStrn, out.bytes, 0x1C);
	argsStrn.clear();

	MaterialCount++;

	return out;
}

void CXMLToMDB::WriteStringToTemp(std::string str)
{
	bool found = false;
	for (size_t strID = 0; strID < m_vecStrns.size(); strID++)
	{
		if (m_vecStrns[strID] == str)
		{
			found = true;
			break;
		}
	}
	if (!found)
		m_vecStrns.push_back(str);
}

MDBMaterialPtr CXMLToMDB::GetMaterialParameter(tinyxml2::XMLElement* entry4)
{
	MDBMaterialPtr out;
	tinyxml2::XMLElement *entry5;
	std::string argsStrn;

	argsStrn = entry4->Attribute("Name");
	WriteStringToTemp(argsStrn);
	out.ptrname = argsStrn;

	entry5 = entry4->FirstChildElement("Color");
	out.r = entry5->FloatAttribute("r");
	out.g = entry5->FloatAttribute("g");
	out.b = entry5->FloatAttribute("b");
	out.a = entry5->FloatAttribute("a");

	out.bytes.resize(0x20);
	memcpy(&out.bytes[0], &out.r, 4U);
	memcpy(&out.bytes[4], &out.g, 4U);
	memcpy(&out.bytes[8], &out.b, 4U);
	memcpy(&out.bytes[12], &out.a, 4U);

	entry5 = entry4->FirstChildElement("raw");
	argsStrn = entry5->GetText();
	WriteRawToByte(argsStrn, out.bytes, 0x10);

	entry5 = entry5->NextSiblingElement("raw");
	argsStrn = entry5->GetText();
	WriteRawToByte(argsStrn, out.bytes, 0x1C);
	
	argsStrn.clear();

	return out;
}

MDBMaterialTex CXMLToMDB::GetMaterialTexture(tinyxml2::XMLElement* entry4, bool NoTexTable)
{
	MDBMaterialTex out;
	tinyxml2::XMLElement* entry5;
	std::wstring wstr1, wstr2;
	std::string argsStrn;
	
	entry5 = entry4->FirstChildElement("Name");
	int nameid = entry5->IntAttribute("MatID");
	int mipmap = entry5->IntAttribute("MIP");
	if (!NoTexTable && nameid)
	{
		out.texid = nameid;
	}
	else
	{
		wstr2 = UTF8ToWide(entry5->GetText());
		wstr1 = wstr2;
		wstr1.replace(wstr1.find_last_of(L"."), 1U, L"_");
		if (mipmap != 0)
		{
			wstr1 += ToString(mipmap);
		}
		// Check if texture exists in table
		bool found = false;
		for (size_t strID = 0; strID < m_vecTexture.size(); strID++)
		{
			if (m_vecTexture[strID].mapping == wstr1)
			{
				found = true;
				out.texid = strID;
				break;
			}
		}
		if (!found)
		{
			out.texid = TextureCount;
			m_vecTexture.push_back( GetTextureInMaterial(wstr1, wstr2) );
		}
	}
	//read type
	entry5 = entry4->FirstChildElement("Type");
	argsStrn = entry5->GetText();
	WriteStringToTemp(argsStrn);
	out.textype = argsStrn;

	out.bytes.resize(0x1C);
	// write id
	memcpy(&out.bytes[0], &out.texid, 4U);
	
	entry5 = entry4->FirstChildElement("raw");
	argsStrn = entry5->GetText();
	WriteRawToByte(argsStrn, out.bytes, 0x8);

	argsStrn.clear();

	return out;
}

MDBTexture CXMLToMDB::GetTextureInMaterial(std::wstring wstr1, std::wstring wstr2)
{
	MDBTexture out;

	out.ID = TextureCount;
	out.mapping = wstr1;
	WriteWStringToTemp(wstr1);
	out.filename = wstr2;
	WriteWStringToTemp(wstr2);

	out.bytes.resize(0x10);
	// write index
	char* idBytes = IntToBytes(out.ID);
	out.bytes[0] = idBytes[0];
	out.bytes[1] = idBytes[1];
	out.bytes[2] = idBytes[2];
	out.bytes[3] = idBytes[3];
	free(idBytes);
	// increment count
	TextureCount++;

	return out;
}

MDBObject CXMLToMDB::GetModel(tinyxml2::XMLElement* entry2, bool NoNameTable, bool multcore)
{
	MDBObject out;
	tinyxml2::XMLElement* entry3;
	int index[4] = { 0 };
	// set object index
	index[0] = ObjectCount;
	out.ID = index[0];
	// get name
	int nameid = entry2->IntAttribute("ID");
	if (!NoNameTable && nameid)
	{
		index[1] = nameid;
	}
	else
	{
		entry3 = entry2->FirstChildElement("name");
		std::wstring wstr = UTF8ToWide(entry3->GetText());
		// Check if name exists in table
		bool found = false;
		for (size_t strID = 0; strID < m_vecWNames.size(); strID++)
		{
			if (m_vecWNames[strID] == wstr)
			{
				found = true;
				index[1] = strID;
				break;
			}
		}
		if (!found)
		{
			index[1] = m_vecWNames.size();
			m_vecWNames.push_back(wstr);
			NameCount++;
		}
		WriteWStringToTemp(wstr);
		wstr.clear();
	}
	out.Nameid = index[1];
	// read mesh count
	for (entry3 = entry2->FirstChildElement("Mesh"); entry3 != 0; entry3 = entry3->NextSiblingElement("Mesh"))
	{
		std::wcout << L"read mesh: " + ToString(index[2]) + L"\n";

		m_vecObjInfo.push_back(GetMeshInModel(entry3, index[2], multcore));
		index[2] += 1;

		std::wcout << L"write mesh complete!\n";
	}
	out.infoCount = index[2];
	out.infoOffset = index[3];

	out.bytes.resize(0x10);
	memcpy(&out.bytes[0], &index, 16U);

	ObjectCount++;

	return out;
}

int CXMLToMDB::GetMeshLayoutSize(tinyxml2::XMLElement* entry5)
{
	int chunkSize, chunkType;
	chunkType = entry5->IntAttribute("type");

	if (chunkType == 1)
		chunkSize = 16;
	else if (chunkType == 4)
		chunkSize = 12;
	else if (chunkType == 7)
		chunkSize = 8;
	else if (chunkType == 12)
		chunkSize = 8;
	else if (chunkType == 21)
		chunkSize = 4;

	return chunkSize;
}

MDBObjectInfo CXMLToMDB::GetMeshInModel(tinyxml2::XMLElement* entry3, int index, bool multcore)
{
	MDBObjectInfo out;
	tinyxml2::XMLElement* entry4, *entry5, *entry6;
	std::string argsStrn;
	std::vector< MDBObjectLayout > objlay;

	out.bytes.resize(0x28);
	// get material id
	out.matid = entry3->IntAttribute("MatID");
	memcpy(&out.bytes[4], &out.matid, 4U);
	// read raw
	entry4 = entry3->FirstChildElement("raw");
	argsStrn = entry4->GetText();
	WriteRawToByte(argsStrn, out.bytes, 0);

	entry4 = entry4->NextSiblingElement("raw");
	argsStrn = entry4->GetText();
	WriteRawToByte(argsStrn, out.bytes, 0x8);
	// get layout
	uint16_t count[2] = { 0 };
	entry4 = entry3->FirstChildElement("VertexList");
	for (entry5 = entry4->FirstChildElement(); entry5 != 0; entry5 = entry5->NextSiblingElement())
	{
		objlay.push_back( GetLayoutInModel(entry5, count[0]) );
		//push bytes
		m_vecObjLayout.push_back(GetLayoutInModel(objlay.back(), 0x10));
		
		count[0] += GetMeshLayoutSize(entry5);
		count[1] += 1;
	}
	out.VertexSize = count[0];
	out.LayoutCount = count[1];
	memcpy(&out.bytes[0x10], &count, 4U);
	// get number of vertices
	int vexNum = 0;
	entry5 = entry4->FirstChildElement();
	for (entry6 = entry5->FirstChildElement("V"); entry6 != 0; entry6 = entry6->NextSiblingElement("V"))
		vexNum++;
	out.VertexNum = vexNum;
	memcpy(&out.bytes[0x14], &out.VertexNum, 4U);
	// write vertex!
	entry4 = entry3->FirstChildElement("VertexList");
	m_vecObjVertices.push_back(GetVerticesInModel(objlay, count[0], entry4, vexNum, count[1], multcore));
	// set index
	out.MeshIndex = index;
	memcpy(&out.bytes[0x18], &out.MeshIndex, 4U);
	// get number of indices
	int InxNum = 0;
	entry4 = entry3->FirstChildElement("Faces");
	for (entry5 = entry4->FirstChildElement("value"); entry5 != 0; entry5 = entry5->NextSiblingElement("value"))
		InxNum++;
	out.indicesNum = InxNum;
	m_vecObjIndices.push_back(GetIndicesInModel(entry4, InxNum));
	memcpy(&out.bytes[0x20], &out.indicesNum, 4U);

	objlay.clear();

	return out;
}

MDBObjectLayout CXMLToMDB::GetLayoutInModel(tinyxml2::XMLElement* entry5, int layoutofs)
{
	MDBObjectLayout out;

	out.type = entry5->IntAttribute("type");
	out.offset = layoutofs;
	out.channel = entry5->IntAttribute("channel");

	std::string argsStrn = entry5->Name();
	WriteStringToTemp(argsStrn);
	out.name = argsStrn;

	out.bytes.resize(0x10);
	memcpy(&out.bytes[0], &out.type, 4U);
	memcpy(&out.bytes[4], &out.offset, 4U);
	memcpy(&out.bytes[8], &out.channel, 4U);

	return out;
}

MDBObjectLayoutOut CXMLToMDB::GetLayoutInModel(MDBObjectLayout objlay, int size)
{
	MDBObjectLayoutOut out;

	out.name = objlay.name;

	out.bytes.resize(size);
	for (int i = 0; i < size; i++)
		out.bytes[i] = objlay.bytes[i];

	return out;
}

MDBByte CXMLToMDB::GetVerticesInModel(std::vector<MDBObjectLayout> objlay, int chunksize, tinyxml2::XMLElement* entry4, int num, int layout, bool multcore)
{
	MDBByte out;
	out.bytes.resize(chunksize * num);

	tinyxml2::XMLElement* entry5;
	entry5 = entry4->FirstChildElement();
	int offset = objlay[0].offset;
	int type = objlay[0].type;
	std::wstring wstr = UTF8ToWide(objlay[0].name);
	std::wcout << L"read: " + wstr + L", ";
	GetModelVertex(type, num, entry5, out.bytes, chunksize, offset);
	std::wcout << L"write complete!\n";
	//start looping to get
	int layoutNum = objlay.size();
	//Of course, starting from 1
	for (int i = 1; i < layoutNum; i++)
	{
		entry5 = entry5->NextSiblingElement();
		offset = objlay[i].offset;
		type = objlay[i].type;
		wstr = UTF8ToWide(objlay[i].name);
		std::wcout << L"read: " + wstr + L", ";
		GetModelVertex(type, num, entry5, out.bytes, chunksize, offset);
		std::wcout << L"write complete!\n";
	}
	//for (entry5 = entry4->FirstChildElement(); entry5 != 0; entry5 = entry5->NextSiblingElement())
	return out;
}

void CXMLToMDB::GetModelVertex(int type, int num, tinyxml2::XMLElement* entry5, std::vector< char >& bytes, int chunksize, int offset)
{
	int pos;
	tinyxml2::XMLElement* entry6 = entry5->FirstChildElement("V");

	if (type == 1)
	{
		float vf[4];

		vf[0] = entry6->FloatAttribute("x");
		vf[1] = entry6->FloatAttribute("y");
		vf[2] = entry6->FloatAttribute("z");
		vf[3] = entry6->FloatAttribute("w");
		memcpy(&bytes[offset], &vf, 16U);

		for (int i = 1; i < num; i++)
		{
			entry6 = entry6->NextSiblingElement("V");
			vf[0] = entry6->FloatAttribute("x");
			vf[1] = entry6->FloatAttribute("y");
			vf[2] = entry6->FloatAttribute("z");
			vf[3] = entry6->FloatAttribute("w");

			pos = offset + (i * chunksize);
			memcpy(&bytes[pos], &vf, 16U);
		}
	}
	else if (type == 4)
	{
		float vf[3];

		vf[0] = entry6->FloatAttribute("x");
		vf[1] = entry6->FloatAttribute("y");
		vf[2] = entry6->FloatAttribute("z");
		memcpy(&bytes[offset], &vf, 12U);

		for (int i = 1; i < num; i++)
		{
			entry6 = entry6->NextSiblingElement("V");
			vf[0] = entry6->FloatAttribute("x");
			vf[1] = entry6->FloatAttribute("y");
			vf[2] = entry6->FloatAttribute("z");

			pos = offset + (i * chunksize);
			memcpy(&bytes[pos], &vf, 12U);
		}
	}
	else if (type == 7)
	{
		half_float::half vf[4];

		vf[0] = entry6->FloatAttribute("x");
		vf[1] = entry6->FloatAttribute("y");
		vf[2] = entry6->FloatAttribute("z");
		vf[3] = entry6->FloatAttribute("w");
		memcpy(&bytes[offset], &vf, 8U);

		for (int i = 1; i < num; i++)
		{
			entry6 = entry6->NextSiblingElement("V");
			vf[0] = entry6->FloatAttribute("x");
			vf[1] = entry6->FloatAttribute("y");
			vf[2] = entry6->FloatAttribute("z");
			vf[3] = entry6->FloatAttribute("w");

			pos = offset + (i * chunksize);
			memcpy(&bytes[pos], &vf, 8U);
		}
	}
	else if (type == 12)
	{
		float vf[2];

		vf[0] = entry6->FloatAttribute("x");
		vf[1] = entry6->FloatAttribute("y");
		memcpy(&bytes[offset], &vf, 8U);

		for (int i = 1; i < num; i++)
		{
			entry6 = entry6->NextSiblingElement("V");
			vf[0] = entry6->FloatAttribute("x");
			vf[1] = entry6->FloatAttribute("y");

			pos = offset + (i * chunksize);
			memcpy(&bytes[pos], &vf, 8U);
		}
	}
	else if (type == 21)
	{
		unsigned char vf[4];

		vf[0] = entry6->IntAttribute("x");
		vf[1] = entry6->IntAttribute("y");
		vf[2] = entry6->IntAttribute("z");
		vf[3] = entry6->IntAttribute("w");
		memcpy(&bytes[offset], &vf, 4U);

		for (int i = 1; i < num; i++)
		{
			entry6 = entry6->NextSiblingElement("V");
			vf[0] = entry6->IntAttribute("x");
			vf[1] = entry6->IntAttribute("y");
			vf[2] = entry6->IntAttribute("z");
			vf[3] = entry6->IntAttribute("w");

			pos = offset + (i * chunksize);
			memcpy(&bytes[pos], &vf, 4U);
		}
	}
}

MDBByte CXMLToMDB::GetIndicesInModel(tinyxml2::XMLElement* entry4, int size)
{
	MDBByte out;
	short value;

	out.bytes.resize(size * 2);

	tinyxml2::XMLElement* entry5 = entry4->FirstChildElement("value");
	value = entry5->IntAttribute("value");
	memcpy(&out.bytes[0], &value, 2U);
	//Here we start at 1 because it is 0 above
	for (int i = 1; i < size; i++)
	{
		entry5 = entry5->NextSiblingElement("value");
		value = entry5->IntAttribute("value");
		//The size of each value is 2, so take x2
		memcpy(&out.bytes[i*2], &value, 2U);
	}

	return out;
}
