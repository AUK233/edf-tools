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

struct MDB_Bone
(
	newBone,

	fn readMatrix4x4 bstream = (
		local mat = matrix3 1
		mat.row1.x = ReadFloat bstream
		mat.row1.y = ReadFloat bstream
		mat.row1.z = ReadFloat bstream
		fseek bstream 4 #seek_cur
		--mat.row1.w = ReadFloat bstream
		mat.row2.x = ReadFloat bstream
		mat.row2.y = ReadFloat bstream
		mat.row2.z = ReadFloat bstream
		fseek bstream 4 #seek_cur
		--mat.row2.w = ReadFloat bstream
		mat.row3.x = ReadFloat bstream
		mat.row3.y = ReadFloat bstream
		mat.row3.z = ReadFloat bstream
		fseek bstream 4 #seek_cur
		--mat.row3.w = ReadFloat bstream
		mat.row4.x = ReadFloat bstream
		mat.row4.y = ReadFloat bstream
		mat.row4.z = ReadFloat bstream
		fseek bstream 4 #seek_cur
		--mat.row4.w = ReadFloat bstream
		mat.row4.x *= 10
		mat.row4.y *= 10
		mat.row4.z *= 10
		mat
	),

	fn readFloatX3 bstream = (
		local f3 = #()
		f3[1] = (ReadFloat bstream) * 10
		f3[2] = (ReadFloat bstream) * 10
		f3[3] = (ReadFloat bstream) * 10
		fseek bstream 4 #seek_cur
		local vf3 = point3 f3[1] f3[2] f3[3]
		vf3
	),
	
	fn readData bstream maxBones namelist index=
	(
		fseek bstream 4 #seek_cur
		local BoneParent = (ReadLong bstream #signed) + 1
		fseek bstream 8 #seek_cur
		local nameIndex = (ReadLong bstream #signed) + 1
		fseek bstream 12 #seek_cur
		local tfm = readMatrix4x4 bstream
		local tfm2 = readMatrix4x4 bstream
		local boxPos = readFloatX3 bstream
		local boxSize = readFloatX3 bstream
		-- create bone
		-- set parent
		if (BoneParent > 0) do (
			tfm = tfm * maxBones[BoneParent].objecttransform
		)
		-- do
		newBone = bonesys.createbone boxPos boxPos [0,0,0.1]
		newBone.name = namelist[nameIndex].UTF16name
		newBone.width  = 0.01
		newBone.height = 0.01
		newBone.transform = tfm
		newBone.setBoneEnable false 0
		newBone.wirecolor = [122,146,87]
		newbone.showlinks = true
		if (BoneParent > 0) then (
			newBone.parent = maxBones[BoneParent]
		)
		maxBones[index] = newBone
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
	pos,
	Material,
	matid,
	LayoutOffset,
	VertexSize,
	LayoutCount,
	VertexNum,
	MeshIndex,
	VertexOffset,
	indicesNum,
	indicesOffset,
	-- vertex type
	position,
	normal,
	blendweight,
	blendindices,
	binormal,
	tangent,
	texcoord0,
	texcoord1,

	fn LoadFromStream bstream=
	(
		pos = ftell bstream
		fseek bstream 4 #seek_cur
		matid = ReadLong bstream #signed
		fseek bstream 4 #seek_cur
		LayoutOffset = ReadLong bstream #signed
		VertexSize = ReadShort bstream #unsigned
		LayoutCount = ReadShort bstream #unsigned
		VertexNum = ReadLong bstream #unsigned
		MeshIndex = ReadLong bstream #signed
		VertexOffset = ReadLong bstream #signed
		indicesNum = (ReadLong bstream #signed) / 3
		indicesOffset = ReadLong bstream #signed
		
		position = 0
		normal = 0
		blendweight = 0
		blendindices = 0
		binormal = 0
		tangent = 0
		texcoord0 = 0
		texcoord1 = 0
	),

	fn GetLayout bstream=
	(
		local curPos = pos + LayoutOffset
		for i = 1 to LayoutCount do (
			fseek bstream (curPos+4) #seek_set
			local layoutOfs = ReadLong bstream #signed
			local channel = ReadLong bstream #signed
			local nameOfs = ReadLong bstream #signed
			-- get str
			fseek bstream (curPos+nameOfs) #seek_set
			local LayoutStr = ReadString bstream
			-- check 
			local dataPos = pos + VertexOffset + layoutOfs
			case LayoutStr of (
				"position": position = dataPos
				"normal": normal = dataPos
				"BLENDWEIGHT": blendweight = dataPos
				"BLENDINDICES": blendindices = dataPos
				"binormal": binormal = dataPos
				"tangent": tangent = dataPos
				"texcoord": (
					if channel == 0 then (
						texcoord0 = dataPos
					) else if channel == 1 then (
						texcoord1 = dataPos
					)
				)
			)
			--format "name: %\n" LayoutStr
			curPos += 16;
		)
	),

	fn ReadMesh bstream meshId index maxBones= (
		-- get faces
		local Vfaces = #()
		local facesPos = pos + indicesOffset
		fseek bstream facesPos #seek_set
		for i = 1 to indicesNum do (
			x = (ReadShort bstream #unsigned) + 1
			y = (ReadShort bstream #unsigned) + 1
			z = (ReadShort bstream #unsigned) + 1
			Vfaces[i] = point3 x y z
		)
		--
		local Vposition = #()
		local curPos = position
		for i = 1 to VertexNum do (
			fseek bstream curPos #seek_set
			x = (ReadFloat bstream) * 10
			y = (ReadFloat bstream) * 10
			z = (ReadFloat bstream) * 10
			Vposition[i] = point3 x y z
			curPos += VertexSize
		)
		--
		local Vnormal = #()
		local curPos = normal
		for i = 1 to VertexNum do (
			fseek bstream curPos #seek_set
			x = ReadFloat bstream
			y = ReadFloat bstream
			z = ReadFloat bstream
			Vnormal[i] = point3 x y z
			curPos += VertexSize
		)
		--
		local Vweights = #()
		if blendweight > 0 then (
			local curPos = blendweight
			for i = 1 to VertexNum do (
				fseek bstream curPos #seek_set
				x = ReadFloat bstream
				y = ReadFloat bstream
				z = ReadFloat bstream
				w = ReadFloat bstream
				Vweights[i] = point4 x y z w
				curPos += VertexSize
			)
		)
		--
		local Vindices = #()
		if blendindices > 0 then (
			local curPos = blendindices
			for i = 1 to VertexNum do (
				fseek bstream curPos #seek_set
				x = (ReadByte bstream #unsigned) + 1
				y = (ReadByte bstream #unsigned) + 1
				z = (ReadByte bstream #unsigned) + 1
				w = (ReadByte bstream #unsigned) + 1
				Vindices[i] = point4 x y z w
				curPos += VertexSize
			)
		)
		--
		local Vtexcoord0 = #()
		local curPos = texcoord0
		for i = 1 to VertexNum do (
			fseek bstream curPos #seek_set
			x = ReadFloat bstream
			y = ReadFloat bstream
			Vtexcoord0[i] = point3 x (1.0-y) 0
			curPos += VertexSize
		)
		--
		local Vtexcoord1 = #()
		if texcoord1 > 0 then (
			local curPos = texcoord1
			for i = 1 to VertexNum do (
				fseek bstream curPos #seek_set
				x = ReadFloat bstream
				y = ReadFloat bstream
				Vtexcoord1[i] = point3 x (1.0-y) 0
				curPos += VertexSize
			)
		)
		--
		local meshName = "nameless"
		if (meshId != "") then
		(
			meshName = meshId
			meshName += "_mesh"
			if ( index < 10 ) then ( meshName += "0" )
			meshName += index as string
		)
		-- create mesh
		local newMesh = mesh name:meshName vertices:Vposition tverts:Vtexcoord0 faces:Vfaces
		-- set User Defined
		setUserProp newMesh "MaterialID" matid
		-- set texcoord
		if (texcoord0 > 0) then
		(
			buildTVFaces newMesh
			for k = 1 to newMesh.numfaces do (setTVFace newMesh k (getFace newMesh k))
		)
		-- set 2
		if (texcoord1 > 0) then
		(
			setUserProp newMesh "Texcoord1" true
			meshop.setNumMaps newMesh 3 keep:true
			meshop.setMapSupport newMesh 2 true
			for k = 1 to VertexNum do
			(
				meshop.setMapVert newMesh 2 k Vtexcoord1[k]
			)
		)
		-- set normal
		for i = 1 to VertexNum do (setNormal newMesh i Vnormal[i])
		max modify mode
		select newMesh

		-- generate skin modifier
		if blendindices > 0 then (
			local skinMod = skin()

			addModifier newMesh skinMod
			
			for k = 1 to maxBones.count do
			(
				if k != maxBones.count then
				(skinOps.addBone skinMod maxBones[k] 0)
				else
				(skinOps.addBone skinMod maxBones[k] 1)
				
				local no = skinOps.getNumberCrossSections skinMod k
				for l=1 to no do
				(
					skinOps.SetInnerRadius skinMod k l 0
					skinOps.SetOuterRadius skinMod k l 0
				)
			)
			-- Set vertex weights
			modPanel.setCurrentObject skinMod
			for k = 1 to VertexNum do (
				bi0 = (Vindices[k].x as integer)
				wv0 = Vweights[k].x
				skinOps.ReplaceVertexWeights skinMod k bi0 wv0
				
				bi1 = (Vindices[k].y as integer)
				wv1 = Vweights[k].y
				skinOps.SetVertexWeights skinMod k bi1 wv1

				bi2 = (Vindices[k].z as integer)
				wv2 = Vweights[k].z
				skinOps.SetVertexWeights skinMod k bi2 wv2

				bi3 = (Vindices[k].w as integer)
				wv3 = Vweights[k].w
				skinOps.SetVertexWeights skinMod k bi3 wv3
			)
			skinOps.RemoveZeroWeights skinMod
		)
		-- end
		update newMesh	
	)
)

struct MDB_Loader
(
	bstream,	--bitsream
	header,		--file header
	namelist=#(),
	texturelist=#(),
	bonelist=#(), --array of bone structs
	objectlist=#(),

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

				--format "name: %\n" namelist[i].UTF16name
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

				--format "textureID: %\ntextureMapping: %\ntextureName: %\n\n" texturelist[i].ID  texturelist[i].Mapping  texturelist[i].UTF16name
			)
		)
		-- get bone
		if header.BoneCount > 0 then
		(
			fseek bstream header.BoneOffset #seek_set
			local temp = MDB_Bone()
			for i = 1 to header.BoneCount do
			(
				temp.readData bstream bonelist namelist i
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

				--format "ModelName: %\n" objectlist[i].ModelName
			)

			-- get mesh
			local temp = MDB_Mesh()
			for i = 1 to header.ObjectCount do
			(
				if objectlist[i].MeshCount > 0 then
				(
					meshlist=#()

					local curPos = header.ObjectOffset + objectlist[i].MeshOffset
					fseek bstream curPos #seek_set
					for j = 1 to objectlist[i].MeshCount do
					(
						temp.LoadFromStream bstream
						append meshlist (copy temp)
					)

					for j = 1 to objectlist[i].MeshCount do
					(
						meshlist[j].GetLayout bstream
						meshlist[j].ReadMesh bstream objectlist[i].ModelName j bonelist
					)

				)
			)
			-- end
		)
	)
)
