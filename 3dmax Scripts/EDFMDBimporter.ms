fileIn "EDFutility.ms"

struct MDB_Header
(
	endian,	-- "MAB\0"
	version,
	NameTableCount,
	NameTableOffset,
	BoneCount,
	BoneOffset,
	ObjectCount,
	ObjectOffset,
	MaterialCount,
	MaterialOffset,
	TextureCount,
	TextureOffset,

	fn LoadFromStream bstream=
	(
		endian = ReadLong bstream #unsigned
		version = ReadLong bstream #unsigned
		NameTableCount = ReadLong bstream #unsigned
		NameTableOffset = ReadLong bstream #signed
		BoneCount = ReadLong bstream #unsigned
		BoneOffset = ReadLong bstream #signed
		ObjectCount = ReadLong bstream #unsigned
		ObjectOffset = ReadLong bstream #signed
		MaterialCount = ReadLong bstream #unsigned
		MaterialOffset = ReadLong bstream #signed
		TextureCount = ReadLong bstream #unsigned
		TextureOffset = ReadLong bstream #signed
	)
)

struct MDB_NameTable
(
	UTF16name,

	fn LoadFromStream  bstream=
	(
		offset = ReadLong bstream #signed
		pos = ftell bstream

		if offset > 0 then
		(
			fseek bstream (pos + offset - 4) #seek_set
			UTF16name = GetWString bstream
		)
		else
		(
			UTF16name = "0"
		)

		fseek bstream pos #seek_set
	)
)

struct MDB_Texture
(
	ID,
	Mapping,
	UTF16name,

	fn LoadFromStream bstream=
	(
		ID = ReadLong bstream #signed
		moffset = ReadLong bstream #signed
		noffset = ReadLong bstream #signed
		unk = ReadLong bstream #unsigned
		
		pos = ftell bstream

		if moffset > 0 then
		(
			fseek bstream (pos + moffset - 16) #seek_set
			Mapping = GetWString bstream
		)
		else
		(
			Mapping = "0"
		)

		if noffset > 0 then
		(
			fseek bstream (pos + noffset - 16) #seek_set
			UTF16name = GetWString bstream
		)
		else
		(
			UTF16name = "0"
		)

		fseek bstream pos #seek_set
	)
)

struct MDB_ObjectTable
(
	ID,
	pos,
	NameIndex,
	MeshCount,
	MeshOffset,
	ModelName,

	fn LoadFromStream bstream=
	(
		pos = ftell bstream
		ID = ReadLong bstream #signed
		NameIndex = ReadLong bstream #signed
		MeshCount = ReadLong bstream #unsigned
		MeshOffset = ReadLong bstream #signed
	)
)

struct MDB_Mesh
(
	Material,
	MaterialID,
	VertexOffset,
	VertexSize,
	VertexLayoutCount,
	VertexNum,
	MeshIndex,
	VertexDataOffset,
	indicesNum,
	indicesOffset,

	fn LoadFromStream bstream=
	(
		pos = ftell bstream
		unk = ReadLong bstream #unsigned
		MaterialID = ReadLong bstream #signed
		unk2 = ReadLong bstream #unsigned
		VertexOffset = ReadLong bstream #signed
		VertexSize = ReadShort bstream #unsigned
		VertexLayoutCount = ReadShort bstream #unsigned
		VertexNum = ReadLong bstream #unsigned
		MeshIndex = ReadLong bstream #signed
		VertexDataOffset = ReadLong bstream #signed
		indicesNum = ReadLong bstream #signed
		indicesOffset = ReadLong bstream #signed
	),

	fn getvalue type=
	(
		if type = 1 then
		(
			y = ReadFloat bstream
			z = ReadFloat bstream
			x = ReadFloat bstream
			w = ReadFloat bstream
		)
		else if type = 4 then
		(
			y = ReadFloat bstream
			z = ReadFloat bstream
			x = ReadFloat bstream
		)
		else if type = 7 then
		(
			--no, it is half float
			y = ReadShort bstream #unsigned
			z = ReadShort bstream #unsigned
			x = ReadShort bstream #unsigned
			w = ReadShort bstream #unsigned
		)
		else if type = 12 then
		(
			x = ReadFloat bstream
			y = ReadFloat bstream
		)
		else if type = 21 then
		(
			y = ReadByte bstream #unsigned
			z = ReadByte bstream #unsigned
			x = ReadByte bstream #unsigned
			w = ReadByte bstream #unsigned
		)
	)
)

struct MDB_BAgroup
(
	ExportedBone,
	LinkBone,
	Type,
	Pos,
	arg1,
	arg2,
	arg3,
	arg4,
	name,

	fn LoadFromStream bstream=
	(
		ExportedBone = ReadLong bstream #unsigned
		LinkBone = ReadLong bstream #unsigned
		Type = ReadLong bstream #unsigned
		Pos = ReadLong bstream #unsigned
		arg1 = ReadLong bstream #unsigned
		arg2 = ReadLong bstream #unsigned
		arg3 = ReadLong bstream #unsigned
		arg4 = ReadLong bstream #unsigned

		if ExportedBone > 0 then
		(
			local temppos = ftell bstream
			fseek bstream ExportedBone #seek_set
			--name = bit.intAsHex ExportedBone
			name = GetWString bstream

			fseek bstream temppos #seek_set
		)
	)
)

struct MDB_Loader
(
	bstream,	--bitsream
	header,		--file header
	namelist=#(),
	texturelist=#(),
	objectlist=#(),
	meshlist=#(),
	bonelist=#(), --array of bone structs

	fn Open filename=
	(
		bstream = fopen filename "rb"
		ok
	),
	
	fn Close=
	(
		fclose bstream
		ok
	),

	fn LoadModel=
	(
		fseek bstream 0 #seek_set
		header = MDB_Header()
		header.LoadFromStream bstream
		-- get name
		if header.NameTableCount > 0 then
		(
			fseek bstream header.NameTableOffset #seek_set
			local temp = MDB_NameTable()
			for i = 1 to header.NameTableCount do
			(
				temp.LoadFromStream bstream
				append namelist (copy temp)

				format "name: %\n" namelist[i].UTF16name
			)
		)
		-- get texture name
		if header.TextureCount > 0 then
		(
			fseek bstream header.TextureOffset #seek_set
			local temp = MDB_Texture()
			for i = 1 to header.TextureCount do
			(
				temp.LoadFromStream bstream
				append texturelist (copy temp)

				format "textureID: %\ntextureMapping: %\ntextureName: %\n\n" texturelist[i].ID  texturelist[i].Mapping  texturelist[i].UTF16name
			)
		)
		-- get object
		if header.ObjectCount > 0 then
		(
			fseek bstream header.ObjectOffset #seek_set
			local temp = MDB_ObjectTable()
			for i = 1 to header.ObjectCount do
			(
				temp.LoadFromStream bstream
				append objectlist (copy temp)

				if objectlist[i].NameIndex >= 0 then
				(
					objectlist[i].ModelName = namelist[objectlist[i].NameIndex+1].UTF16name
				)

				format "ModelName: %\n" objectlist[i].ModelName
			)

			-- get mesh
			local temp = MDB_Mesh()
			for i = 1 to header.ObjectCount do
			(
				if objectlist[i].MeshOffset > 0 then
				(
					fseek bstream (header.ObjectOffset + objectlist[i].MeshOffset) #seek_set
					temp.LoadFromStream bstream
					append meshlist (copy temp)
				)
			)
		)
	)
)
