struct EDFMABPoint
(
	fn MABPoint=
	(	
		Output = newScript()
		
		if selection.count > 0 then
		(
			bname = #( )
			bpar = #( )
			bpos = #( )
			brat = #( )
			pname = #( )

			for i = 1 to selection.count do
			(
				local type = getUserProp selection[i] "MABPoint"
				if type == true then
				(
					local x, y, z
					bname[i] = selection[i].name
					bpar[i] = selection[i].parent
					-- read position
					if bpar[i] != undefined then
					(
						x = (selection[i].position.x - bpar[i].position.x) / 10.0
						y = (selection[i].position.y - bpar[i].position.y) / 10.0
						z = (selection[i].position.z - bpar[i].position.z) / 10.0
						pname[i] = bpar[i].name
					)
					else
					(
						x = selection[i].position.x / 10.0
						y = selection[i].position.y / 10.0
						z = selection[i].position.z / 10.0
						pname[i] = "mdl"
					)
					bpos[i] = point3 x y z
					--
					if bpar[i] != undefined then
					(
						x = (selection[i].rotation.x - bpar[i].rotation.x) * 3.14
						y = (selection[i].rotation.y - bpar[i].rotation.y) * 3.14
						z = (selection[i].rotation.z - bpar[i].rotation.z) * 3.14
					)
					else
					(
						x = selection[i].rotation.x * 3.14
						y = selection[i].rotation.y * 3.14
						z = selection[i].rotation.z * 3.14
					)
					brat[i] = point3 x y z

					format "                <ptr ExportBone=\"%\" Parent=\"%\"\n" bname[i] pname[i] to:Output
					format "                    location:\n"to:Output
					format "                        <value>%</value>\n                        <value>%</value>\n                        <value>%</value>\n" bpos[i].x bpos[i].y bpos[i].z to:Output
					format "                    rotation:\n"to:Output
					format "                        <value>%</value>\n                        <value>%</value>\n                        <value>%</value>\n\n" brat[i].x brat[i].y brat[i].z to:Output
				)
			)
		) -- end if
		else
		(
			messagebox "Error: No Objects Selected." title:"Error"
		)
	)
)