fileIn "EDFutility.ms"

struct MDB_TXTExporter
(
	output_file,
	boneEnd,
	maxBones = #(),
	multimat,
	maxObjects = #(),

	fn OutputModel output_file=
	(
		--objectnum = 0
		for Object in $*_mesh01 do
		(
			local meshname = Object.name
			local objectname = substring meshname 1 ((findString meshname "_mesh")-1)
			--meshid = substring meshname ((findString meshname "_mesh")+4) 2

			if (objectname != "") then
			(
				format "        <Object>\n            <name>%</name>\n" objectname to:output_file
				--
				for Object in $*_mesh* do
				(
					local mname = Object.name
					local objname = substring mname 1 ((findString mname "_mesh")-1)
					-- Make sure it's the same object
					if (objname == objectname) then
					(
						local vertsnum = Object.numverts
						local meshverts = #()
						local meshnrms = #()
						local meshbins = #()
						local meshtans = #()
						local meshtexs0 = #()
						local meshtexs1 = #()
						--Check the skin state at start
						local skinstate = 0
						local skinbonel = 0
						try
						(
							-- skin
							local skinMod = Object.modifiers[#Skin]
							--select Object
							max modify mode
							modPanel.setCurrentObject skinMod node:Object ui:true
							local nbones = skinOps.GetVertexWeightCount skinMod 1
							local wbone1 = #()
							local wbone2 = #()
							local wbone3 = #()
							local wbone4 = #()
							local ibone1 = #()
							local ibone2 = #()
							local ibone3 = #()
							local ibone4 = #()
							for i = 1 to vertsnum do
							(
								nbones = skinOps.GetVertexWeightCount skinMod i
								wbone1[i] = skinOps.GetVertexWeight skinMod i 1
								ibone1[i] = (skinOps.GetVertexWeightBoneID skinMod i 1) - 1
								if(skinbonel < 1) then (skinbonel += 1)
								
								if (nbones>=2) then (
									wbone2[i] = skinOps.GetVertexWeight skinMod i 2
									ibone2[i] = (skinOps.GetVertexWeightBoneID skinMod i 2) - 1
									if(skinbonel < 2) then (skinbonel += 1)
								) else (
									wbone2[i] = 0
									ibone2[i] = 0
								)

								if (nbones>=3) then (
									wbone3[i] = skinOps.GetVertexWeight skinMod i 3
									ibone3[i] = (skinOps.GetVertexWeightBoneID skinMod i 3) - 1
									if(skinbonel < 3) then (skinbonel += 1)
								) else (
									wbone3[i] = 0
									ibone3[i] = 0
								)
								--Has up to 4 bone effects
								if (nbones>=4) then (
									wbone4[i] = skinOps.GetVertexWeight skinMod i 4
									ibone4[i] = (skinOps.GetVertexWeightBoneID skinMod i 4) - 1
									if(skinbonel < 4) then (skinbonel += 1)
								) else (
									wbone4[i] = 0
									ibone4[i] = 0
								)

								skinstate = 1
							)
						)
						catch
						(
							skinstate = 0
							skinbonel = 0
						)
						local matid = getUserProp Object "MaterialID"
						if (matid == undefined) then ( matid = 0 ) 
						format "            <Mesh MatID=\"%\">\n                <raw>000%0%00</raw>\n                <raw>00000000</raw>\n" matid skinstate skinbonel to:output_file
						-- vertex
						format "                <VertexList>\n" to:output_file
						-- position
						format "                    <position type=\"7\" channel=\"0\">\n                        <debug/>\n" to:output_file
						for i = 1 to vertsnum do
						(
							-- get all data
							meshverts[i] = getVert Object i
							meshverts[i] /= 10.0
							meshnrms[i] = getNormal Object i
							meshtexs0[i] = getTVert Object i
							-- calculate tangent
							meshtans[i] = normalize (meshverts[i] / (meshtexs0[i].x - 1.0 + meshtexs0[i].y))
							--meshbins[i] = normalize ([meshverts[i].z, meshverts[i].x, meshverts[i].y] / meshtexs0[i].x)
							-- calculate binormal
							meshbins[i] = cross meshnrms[i] meshtans[i]
							-- out position
							format "                        <V x=\"%\" y=\"%\" z=\"%\" w=\"1\"/>\n" meshverts[i].x meshverts[i].y meshverts[i].z to:output_file
						)
						format "                    </position>\n" to:output_file
						-- normal
						format "                    <normal type=\"7\" channel=\"0\">\n                        <debug/>\n" to:output_file
						for i = 1 to vertsnum do
						(
							format "                        <V x=\"%\" y=\"%\" z=\"%\" w=\"1\"/>\n" meshnrms[i].x meshnrms[i].y meshnrms[i].z to:output_file
						)
						format "                    </normal>\n" to:output_file
						-- skin
						if (ibone1 != undefined) then
						(
							-- bone weight
							format "                    <BLENDWEIGHT type=\"1\" channel=\"0\">\n                        <debug/>\n" to:output_file
							for i = 1 to vertsnum do
							(
								format "                        <V x=\"%\" y=\"%\" z=\"%\" w=\"%\"/>\n" wbone1[i] wbone2[i] wbone3[i] wbone4[i] to:output_file
							)
							format "                    </BLENDWEIGHT>\n" to:output_file
							-- bone indice
							format "                    <BLENDINDICES type=\"21\" channel=\"0\">\n                        <debug/>\n" to:output_file
							for i = 1 to vertsnum do
							(
								format "                        <V x=\"%\" y=\"%\" z=\"%\" w=\"%\"/>\n" ibone1[i] ibone2[i] ibone3[i] ibone4[i] to:output_file
							)
							format "                    </BLENDINDICES>\n" to:output_file
						)
						-- binormal
						format "                    <binormal type=\"7\" channel=\"0\">\n                        <debug/>\n" to:output_file
						for i = 1 to vertsnum do
						(
							format "                        <V x=\"%\" y=\"%\" z=\"%\" w=\"1\"/>\n" meshbins[i].x meshbins[i].y meshbins[i].z to:output_file
							--format "                        <V x=\"%\" y=\"%\" z=\"%\" w=\"1\"/>\n" -meshnrms[i].y meshnrms[i].x meshnrms[i].z to:output_file
						)
						format "                    </binormal>\n" to:output_file
						-- tangent
						format "                    <tangent type=\"7\" channel=\"0\">\n                        <debug/>\n" to:output_file
						for i = 1 to vertsnum do
						(
							format "                        <V x=\"%\" y=\"%\" z=\"%\" w=\"1\"/>\n" meshtans[i].x meshtans[i].y meshtans[i].z to:output_file
						)
						format "                    </tangent>\n" to:output_file
						-- texcoord0
						--selection[1].numtverts
						--selection[1].numverts
						format "                    <texcoord type=\"12\" channel=\"0\">\n                        <debug/>\n" to:output_file
						for i = 1 to vertsnum do
						(
							--import is flipped, so export is flipped back
							format "                        <V x=\"%\" y=\"%\"/>\n" meshtexs0[i].x (1.0-meshtexs0[i].y) to:output_file
						)
						format "                    </texcoord>\n" to:output_file
						-- texcoord1
						local hastex1 = getUserProp Object "Texcoord1"
						if (hastex1 == true) then
						(
							format "                    <texcoord type=\"12\" channel=\"1\">\n                        <debug/>\n" to:output_file
							for i = 1 to vertsnum do
							(
								meshtexs1[i] = meshop.getMapVert Object 2 i
								--import is flipped, so export is flipped back
								format "                        <V x=\"%\" y=\"%\"/>\n" meshtexs1[i].x (1.0-meshtexs1[i].y) to:output_file
							)
							format "                    </texcoord>\n" to:output_file
						)
						-- 
						format "                </VertexList>\n" to:output_file

						local facesnum = Object.numfaces
						local facesvert = #()
						format "                <Faces>\n" to:output_file
						for i = 1 to facesnum do
						(
							facesvert[i] = getFace Object i
							format "                    <value value=\"%\"/>\n" ((facesvert[i].x - 1) as integer) to:output_file
							format "                    <value value=\"%\"/>\n" ((facesvert[i].y - 1) as integer) to:output_file
							format "                    <value value=\"%\"/>\n" ((facesvert[i].z - 1) as integer) to:output_file
						)
						format "                </Faces>\n" to:output_file

						format "            </Mesh>\n" to:output_file
					)
				)

				format "        </Object>\n" to:output_file
			)
		)
	), 

	fn ExportModel output_name=
	(
		--output_file = createfile output_name
		--output_file = openFile output_name mode:"wb"

		-- write data to buffer
		output_file = stringstream ""
		format "<MDB>\n" to:output_file

		format "    <ObjectLists>\n" to:output_file
		OutputModel output_file
		format "    </ObjectLists>\n" to:output_file

		format "</MDB>" to:output_file
		-- end

		-- write to file
		out = openFile output_name mode:"wb"
		print (output_file as string) to:out
		close output_file
		close out

		gc()
	)
)
