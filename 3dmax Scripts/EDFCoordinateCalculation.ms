struct EDFCoordinate_Out
(
	fn EDFCoordinate output_name=
	(	
		if selection.count > 0 then
		(
			posmanx = #( )
			posmany = #( )
			posmanz = #( )
			nameman = #( )
			-- route group
			if output_name != undefined then 
			(
				output_file = createfile output_name
				route = 0
				format "\"base data\": [\n" to:output_file
				for i = 1 to selection.count do
				(
					posmanx[i] = selection[i].position.y
					posmany[i] = selection[i].position.z
					posmanz[i] = selection[i].position.x
					nameman[i] = selection[i].name
					if classOf selection[i].baseObject == dummy then
					(
						format "{\n\"name\": \"%\",\n" nameman[i] to:output_file
						format "\"positions\": [\n%,\n%,\n%\n],\n" posmanx[i] posmany[i] posmanz[i] to:output_file
						format "\"rmpa_float_WayPointWidth\": -1.0,\n" route to:output_file
						format "\"route number\": %,\n" route to:output_file
						format "\"current->next number\": []\n},\n" to:output_file
						route += 1
					)
				)
				format "]\n" to:output_file
				-- shape group
				format "\n-----------\n\n" to:output_file

				format "\"base data\": [\n" to:output_file
				for i = 1 to selection.count do
				(
					posmanx[i] = selection[i].position.y
					posmany[i] = selection[i].position.z
					posmanz[i] = selection[i].position.x
					nameman[i] = selection[i].name
					if classOf selection[i].baseObject == Sphere then
					(
						radius = #( )
						--
						radius[i] = selection[i].radius
						format "{\n\"shape type name\": \"Sphere\",\n\"shape var name\": \"%\",\n" nameman[i] to:output_file
						format "\"shape positions data\": [\n%,\n%,\n%,\n0.0,\n0.0,\n0.0,\n0.0,\n0.0,\n%,\n0.0\n]\n" posmanx[i] posmany[i] posmanz[i] radius[i] to:output_file
						format "},\n" to:output_file
					)

					if classOf selection[i].baseObject == Cylinder then
					(
						radius = #( )
						height = #( )
						--
						radius[i] = selection[i].radius
						height[i] = selection[i].height
						format "{\n\"shape type name\": \"Cylinder\",\n\"shape var name\": \"%\",\n" nameman[i] to:output_file
						format "\"shape positions data\": [\n%,\n%,\n%,\n0.0,\n0.0,\n0.0,\n0.0,\n0.0,\n%,\n%\n]\n" posmanx[i] posmany[i] posmanz[i] radius[i] height[i] to:output_file
						format "},\n" to:output_file
					)
				)
				format "]\n" to:output_file
				-- point group
				format "\n-----------\n\n" to:output_file

				format "\"base data\": [\n" to:output_file
				for i = 1 to selection.count do
				(
					posmanx[i] = selection[i].position.y
					posmany[i] = selection[i].position.z
					posmanz[i] = selection[i].position.x
					nameman[i] = selection[i].name
					if classOf selection[i].baseObject == point then
					(
						rotormanx = #( )
						rotormany = #( )
						rotormanz = #( )
						--
						rotormanx[i] = posmanx[i] + 20 * cos ( selection[i].rotation.z_rotation )
						rotormany[i] = posmany[i]
						rotormanz[i] = posmanz[i] - 20 * sin ( selection[i].rotation.z_rotation )
						format "{\n\"name\": \"%\",\n" nameman[i] to:output_file
						format "\"positions_1\": [\n%,\n%,\n%\n],\n" posmanx[i] posmany[i] posmanz[i] to:output_file
						format "\"positions_2\": [\n%,\n%,\n%\n]\n" rotormanx[i] rotormany[i] rotormanz[i] to:output_file
						format "},\n" to:output_file
					)
				)
				format "]\n" to:output_file

				close output_file
			) --end if
		) --end if
		else
		(
			messagebox "Error: No Objects Selected." title:"Error"
		)
	)
)