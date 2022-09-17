struct EDFFixVertUV
(
	fn FixUV=
	(	
		Output = newScript()

		if selection.count > 0 then
		(
			for i = 1 to selection.count do
			(
				if ClassOf selection[i].baseObject == Editable_mesh then
				(
					local vertuv = #( )
					local nmesh = selection[i].mesh
					local facenum = getNumFaces nmesh
					for k = 1 to facenum do
					(
						local TVface = getTVFace nmesh k
						local Vface = getFace nmesh k
						-- the two may be different
						vertuv[Vface.x] = getTVert nmesh TVface.x
						vertuv[Vface.y] = getTVert nmesh TVface.y
						vertuv[Vface.z] = getTVert nmesh TVface.z
						-- so all it takes is to make them the same!
					)
					local vertsnum = nmesh.numverts
					FORMAT "%\nnumber of vertices required:%\nnumber of vertices output:%\n" selection[i].name vertsnum vertuv.count to:Output
					
					nmesh.numTVerts = vertsnum
					buildTVFaces nmesh

					for k = 1 to vertsnum do
					(
						if (vertuv[k] == undefined) then (vertuv[k] = [0,0,0])
						setTVert nmesh k vertuv[k]
					)
					--It is necessary
					for k = 1 to facenum do (setTVFace nmesh k (getFace nmesh k))
				)
			)
			messagebox "Finished UV fix for vertices." title:"Success"
		) -- end if
		else
		(
			messagebox "Error: No Objects Selected." title:"Error"
		)
	)
)