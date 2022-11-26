fileIn "EDFutility.ms"

struct MDB_TXTLoader
(
	file,
	path,
	boneEnd,
	maxBones = #(),
	multimat,
	maxObjects = #(),

	fn Open filename=
	(
		file = openFile filename
		path = getFilenamePath filename
		ok
	),
	
	fn Close=
	(
		close
		ok
	),

	fn ReadBone file=
	(
		m11 = 1; m12 = 0; m13 = 0; m14 = 0
		m21 = 0; m22 = 1; m23 = 0; m24 = 0
		m31 = 0; m32 = 0; m33 = 1; m34 = 0
		m41 = 0; m42 = 0; m43 = 0; m44 = 1
		m15 = 1; m16 = 0; m17 = 0; m18 = 0
		m25 = 0; m26 = 1; m27 = 0; m28 = 0
		m35 = 0; m36 = 0; m37 = 1; m38 = 0
		m45 = -0; m46 = -0; m47 = -0; m48 = 1
		posX = 0; posY = 0; posZ = 0
		rotX = 0; rotY = 0; rotZ = 0
		
		seek file 0
		skipToString file "<BoneLists"
		skipToString file "count=\""
		local num = readDelimitedString file "\"" as integer
		ikx = #()
		iky = #()
		
		for i = 1 to num do
		(
			skipToString file "<Bone>"
			skipToString file "<name"
			skipToString file ">"
			local BoneName = readDelimitedString file "<"
			-- get parent
			skipToString file "<parent value=\""
			BoneParent = (readDelimitedString file "\"" as integer) + 1
			-- get ik
			skipToString file "<IK"
			skipToString file "root=\""
			ikx[i] = (readDelimitedString file "\"" as integer) + 1
			skipToString file "next=\""
			iky[i] = (readDelimitedString file "\"" as integer) + 1
			-- get main transformation matrix
			skipToString file "<mainTM"
			skipToString file "x=\""
			m11 = readDelimitedString file "\"" as float
			skipToString file "y=\""
			m12 = readDelimitedString file "\"" as float
			skipToString file "z=\""
			m13 = readDelimitedString file "\"" as float
			skipToString file "w=\""
			m14 = readDelimitedString file "\"" as float
			skipToString file "<mainTM"
			skipToString file "x=\""
			m21 = readDelimitedString file "\"" as float
			skipToString file "y=\""
			m22 = readDelimitedString file "\"" as float
			skipToString file "z=\""
			m23 = readDelimitedString file "\"" as float
			skipToString file "w=\""
			m24 = readDelimitedString file "\"" as float
			skipToString file "<mainTM"
			skipToString file "x=\""
			m31 = readDelimitedString file "\"" as float
			skipToString file "y=\""
			m32 = readDelimitedString file "\"" as float
			skipToString file "z=\""
			m33 = readDelimitedString file "\"" as float
			skipToString file "w=\""
			m34 = readDelimitedString file "\"" as float
			skipToString file "<mainTM"
			skipToString file "x=\""
			m41 = (readDelimitedString file "\"" as float) * 10
			skipToString file "y=\""
			m42 = (readDelimitedString file "\"" as float) * 10
			skipToString file "z=\""
			m43 = (readDelimitedString file "\"" as float) * 10
			skipToString file "w=\""
			m44 = readDelimitedString file "\"" as float
			-- get skin transformation matrix
			skipToString file "<skinTM"
			skipToString file "x=\""
			m15 = readDelimitedString file "\"" as float
			skipToString file "y=\""
			m16 = readDelimitedString file "\"" as float
			skipToString file "z=\""
			m17 = readDelimitedString file "\"" as float
			skipToString file "w=\""
			m18 = readDelimitedString file "\"" as float
			skipToString file "<skinTM"
			skipToString file "x=\""
			m25 = readDelimitedString file "\"" as float
			skipToString file "y=\""
			m26 = readDelimitedString file "\"" as float
			skipToString file "z=\""
			m27 = readDelimitedString file "\"" as float
			skipToString file "w=\""
			m28 = readDelimitedString file "\"" as float
			skipToString file "<skinTM"
			skipToString file "x=\""
			m35 = readDelimitedString file "\"" as float
			skipToString file "y=\""
			m36 = readDelimitedString file "\"" as float
			skipToString file "z=\""
			m37 = readDelimitedString file "\"" as float
			skipToString file "w=\""
			m38 = readDelimitedString file "\"" as float
			skipToString file "<skinTM"
			skipToString file "x=\""
			m45 = (readDelimitedString file "\"" as float) * 10
			skipToString file "y=\""
			m46 = (readDelimitedString file "\"" as float) * 10
			skipToString file "z=\""
			m47 = (readDelimitedString file "\"" as float) * 10
			skipToString file "w=\""
			m48 = readDelimitedString file "\"" as float
			-- get position
			skipToString file "<position"
			skipToString file "x=\""
			posX = (readDelimitedString file "\"" as float) * 10
			skipToString file "y=\""
			posY = (readDelimitedString file "\"" as float) * 10
			skipToString file "z=\""
			posZ = (readDelimitedString file "\"" as float) * 10
			-- get rotation
			skipToString file "<float"
			skipToString file "x=\""
			rotX = readDelimitedString file "\"" as float
			skipToString file "y=\""
			rotY = readDelimitedString file "\"" as float
			skipToString file "z=\""
			rotZ = readDelimitedString file "\"" as float
			-- create bone
			tfm = matrix3 [m11,m12,m13] [m21,m22,m23] [m31,m32,m33] [m41,m42,m43]
			-- Abandon matrix 2, even if he is useful for skinning
			tfm2 = matrix3 [m15,m16,m17] [m25,m26,m27] [m35,m36,m37] [m45,m46,m47]
			--tfm2 = inverse tfm2
			/*
			tfm.row1 = tfm.row1 * tfm2.row1
			tfm.row2 = tfm.row2 * tfm2.row2
			tfm.row3 = tfm.row3 * tfm2.row3
			tfm.row4 = tfm.row4 + [m45,m46,m47]
			*/
			--tfm = tfm * tfm2
			-- set parent
			if (BoneParent > 0) do (
				tfm = tfm * maxBones[BoneParent].objecttransform
			)
			-- do
			newBone = bonesys.createbone [posX,posY,posZ] [posX,posY,posZ] [0,0,0.1]
			newBone.name = BoneName
			newBone.width  = 0.01
			newBone.height = 0.01
			newBone.transform = tfm
			--newBone.scale = [m11, m22, m33]
			newBone.setBoneEnable false 0
			newBone.wirecolor = [122,146,87]
			newbone.showlinks = true
			--newBone.pos += [posX,posY,posZ]
			--newBone.rotation += Quat rotX rotY rotZ 1
			--newBone.pos.controller      = TCB_position ()
			--newBone.rotation.controller = TCB_rotation ()
			if (BoneParent > 0) then (newBone.parent = maxBones[BoneParent])
			maxBones[i] = newBone
		)
		
		boneEnd = filePos file
		-- There is a problem
		/*
		for i = 1 to maxBones.count do
		(
			r = ikx[i]
			n = iky[i]
			--if (r > 0) then ( ik.setPinNode maxBones[i] maxBones[r] )
			if (n > 0) then ( ik.setPinNode maxBones[n] maxBones[i] )

			--HDIKSys.ikChain maxBones[sbone] maxBones[ebone] true
			--IKSys.ikChain maxBones[sbone] maxBones[ebone] "IKHISolver"
		)
		*/
	),

	fn ReadMaterial file path=
	(
		seek file boneEnd
		skipToString file "<Materials"
		skipToString file "count=\""
		local num = readDelimitedString file "\"" as integer

		multimat = MultiMaterial()
		multimat.name = "edf"
		multimat.numsubs = num

		for i = 1 to num do
		(
			skipToString file "<MaterialNode"
			skipToString file "<MaterialName"
			skipToString file ">"
			local matId = readDelimitedString file "<"

			skipToString file "<Texture"
			skipToString file "<Name"
			skipToString file ">"
			local tex = readDelimitedString file "<"
			--try to get the texture path
			try (
				texpath = substring path 1 ((findString path "Exclude")-1)
				texpath += "HD-TEXTURE/"
			) catch (
				texpath = path
			)
			
			mat = multimat.materialList[i]
			mat.name = matId
			mat.showinviewport = true
			tm = Bitmaptexture filename:(texpath + tex)
			tm.alphasource = 0
			mat.diffuseMap = tm
			--mat.opacityMap = tm
			--mat.opacityMap.monoOutput = 1
		)
	),

	fn ReadObject file=
	(
		seek file boneEnd
		skipToString file "<ObjectLists"
		skipToString file "count=\""
		local num = readDelimitedString file "\"" as integer

		local meshStart = filePos file
		for i = 1 to num do
		(
			skipToString file "<Object"
			skipToString file "count=\""
			local meshnum = readDelimitedString file "\"" as integer
			-- get name
			skipToString file "<name>"
			local meshId = readDelimitedString file "<"

			maxMesh = #()
			meshStart = filePos file
			for j = 1 to meshnum do
			(
				seek file meshStart
				skipToString file "<Mesh"
				skipToString file "MatID=\""
				local matid = (readDelimitedString file "\"" as integer) + 1

				skipToString file "<VertexList"
				skipToString file "Count=\""
				local vnum = readDelimitedString file "\"" as integer
				
				local x, y, z
				local tagStart = filePos file
				--
				local position = #()
				seek file tagStart
				skipToString file "<position"
				for k = 1 to vnum do
				(
					skipToString file "<V"
					skipToString file "x=\""
					x = (readDelimitedString file "\"" as float) * 10
					skipToString file "y=\""
					y = (readDelimitedString file "\"" as float) * 10
					skipToString file "z=\""
					z = (readDelimitedString file "\"" as float) * 10
					position[k] = point3 x y z
				)

				local normal = #()
				seek file tagStart
				skipToString file "<normal"
				for k = 1 to vnum do
				(
					skipToString file "<V"
					skipToString file "x=\""
					x = readDelimitedString file "\"" as float
					skipToString file "y=\""
					y = readDelimitedString file "\"" as float
					skipToString file "z=\""
					z = readDelimitedString file "\"" as float
					normal[k] = point3 x y z
				)

				local weights = #()
				seek file tagStart
				local weightsexist = skipToString file "<BLENDWEIGHT"
				if ( weightsexist != undefined ) then
				(
					local w = 0
					for k = 1 to vnum do
					(
						skipToString file "<V"
						skipToString file "x=\""
						x = readDelimitedString file "\"" as float
						skipToString file "y=\""
						y = readDelimitedString file "\"" as float
						skipToString file "z=\""
						z = readDelimitedString file "\"" as float
						skipToString file "w=\""
						w = readDelimitedString file "\"" as float
						weights[k] = point4 x y z w
					)
				)

				local indices = #()
				seek file tagStart
				local indicesexist = skipToString file "<BLENDINDICES"
				if ( indicesexist != undefined ) then
				(
					local w = 0
					for k = 1 to vnum do
					(
						skipToString file "<V"
						skipToString file "x=\""
						x = readDelimitedString file "\"" as integer
						skipToString file "y=\""
						y = readDelimitedString file "\"" as integer
						skipToString file "z=\""
						z = readDelimitedString file "\"" as integer
						skipToString file "w=\""
						w = readDelimitedString file "\"" as integer
						indices[k] = point4 (x+1) (y+1) (z+1) (w+1)
					)
				)

				local texcoord0 = #()
				seek file tagStart
				skipToString file "<texcoord"
				skipToString file "channel=\"0\""
				for k = 1 to vnum do
				(
					skipToString file "<V"
					skipToString file "x=\""
					x = readDelimitedString file "\"" as float
					skipToString file "y=\""
					y = readDelimitedString file "\"" as float
					--Flip Y for normal display
					texcoord0[k] = point3 x (1.0-y) 0
				)
				local texEnd0 = filePos file
				--
				skipToString file "<Faces"
				local tagEnd = filePos file
				skipToString file "Count=\""
				local inum = readDelimitedString file "\"" as integer
				local faces = #()
				for k = 1 to inum / 3 do
				(
					skipToString file "<value"
					skipToString file "value=\""
					x = readDelimitedString file "\"" as integer
					skipToString file "<value"
					skipToString file "value=\""
					y = readDelimitedString file "\"" as integer
					skipToString file "<value"
					skipToString file "value=\""
					z = readDelimitedString file "\"" as integer
					faces[k] = point3 (1 + x) (1 + y) (1 + z)
				)
				meshStart = filePos file
				-- try channel 1
				local texcoord1 = #()
				seek file texEnd0
				try (
					skipToString file "<texcoord"
					skipToString file "channel=\"1\""
					if (filePos file < tagEnd) then
					(
						for k = 1 to vnum do
						(
							skipToString file "<V"
							skipToString file "x=\""
							x = readDelimitedString file "\"" as float
							skipToString file "y=\""
							y = readDelimitedString file "\"" as float
							--Flip Y for normal display
							texcoord1[k] = point3 x (1.0-y) 0
						)
					)
					else( seek file meshStart )
				) catch ()
				--
				if (meshId != "") then
				(
					meshName = meshId
					meshName += "_mesh"
					if ( j < 10 ) then ( meshName += "0" )
					meshName += j as string
				)

				local newMesh = mesh name:meshName vertices:position tverts:texcoord0 faces:faces
				newMesh.material = multimat.materialList[matid]
				-- set User Defined
				setUserProp newMesh "MaterialID" (matid - 1)

				if (texcoord0.count > 0) then
				(
					buildTVFaces newMesh
					for k = 1 to newMesh.numfaces do (setTVFace newMesh k (getFace newMesh k))
				)

				if (texcoord1.count > 0) then
				(
					setUserProp newMesh "Texcoord1" true
					meshop.setNumMaps newMesh 3 keep:true
					meshop.setMapSupport newMesh 2 true
					--for k = 1 to newMesh.numfaces do (meshop.setNumMapFaces newMesh 2 k keep:true keepCount:2)
					for k = 1 to vnum do
					(
						meshop.setMapVert newMesh 2 k texcoord1[k]
					)
				)
				
				for k = 1 to vnum do (setNormal newMesh k normal[k])
				max modify mode
				select newMesh

				-- generate skin modifier
				if (indices.count > 0) then
				(		
					local skinMod = skin()
					--skinMod.bone_Limit = 4

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
					for k = 1 to vnum do (
						--This will clear all weights and set only one bone
						--skinOps.ReplaceVertexWeights skinMod k indices[k] weights[k]
						
						bi0 = (indices[k].x as integer)
						wv0 = weights[k].x
						skinOps.ReplaceVertexWeights skinMod k bi0 wv0
						
						bi1 = (indices[k].y as integer)
						wv1 = weights[k].y
						skinOps.SetVertexWeights skinMod k bi1 wv1

						bi2 = (indices[k].z as integer)
						wv2 = weights[k].z
						skinOps.SetVertexWeights skinMod k bi2 wv2

						bi3 = (indices[k].w as integer)
						wv3 = weights[k].w
						skinOps.SetVertexWeights skinMod k bi3 wv3
					)
					skinOps.RemoveZeroWeights skinMod
				)
				update newMesh	
				
				maxMesh[j] = newMesh
			)
		)
	),

	fn LoadModel=
	(
		seek file 0
		ReadBone file
		ReadMaterial file path
		ReadObject file
	)
)
