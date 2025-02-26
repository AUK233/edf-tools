/*
========================================
========================================
*/

fileIn "EDFMABimporter.ms"
fileIn "EDFMACimporter.ms"
fileIn "EDFMDBimporter.ms"
fileIn "EDFMDBtxtimporter.ms"
-- exporter
fileIn "EDFCoordinateCalculation.ms"
fileIn "EDFMDBtxtexporter.ms"
fileIn "EDFMABPointCalculation.ms"
-- other
fileIn "EDFFixVertexUV.ms"

utility EDFTool "Sandlot EDF" silentErrors:false
(
	--Rollout
	group "About:"
	( 
		label Label1 "Sandlot EDF MaxTool"
		label Label2 "02-12"
	)
	
	group "Import:"
	(
		label labelod "Only Data:"
		button importMAB "Import .MAB" tooltip:"Import mab"
		button importMAC "Import .MAC" tooltip:"Import coordinates of map objects in MAC."
		
		label labelm "Model:"
		button importMDB "Import .MDB" tooltip:"Import Model. It requires that you update mdb."
		button importMDBtxt "Import MDB text" tooltip:"Import model from xml format."
	)
	
	group "Export:"
	(
		button OutputCoord "Output Coord" tooltip:"Output the location of the object on the map."

		label labelexm "Model:"
		button exportMDBtxt "Export MDB text" tooltip:"Export model to xml format. Only models can now be exported."
		button OutputMABPoint "Output MAB Point" tooltip:"Export bone information for MAB mapping."
	)

	group "Other:"
	(
		button FixVertUV "Fix vertex UVs" tooltip:"Fix the UV coordinates to each vertex so that the exported model texture is correct."
	)

	-- Utilities --
	fn DoesFileReallyExist filename=
	(
		local temp = fopen filename "rb"
		if temp != undefined then
		(
			fclose temp
			true
		)
		else
		(
			false
		)
	)
	
	--UI Stuff

	-- input

	on importMAB pressed do
	(
		inipath = scriptsPath + "settings.ini"
		filename = getINISetting inipath "settings" "importmab"
	    mabFilename = getOpenFileName caption:"Select MAB file" \
					filename:filename \
					types:"EDF MAB (*.mab)|*.mab|EDF HEX (*.hex)|*.hex|All Files (*.*)|*.*|"

		if mabFilename != undefined then
		(
			if DoesFileReallyExist mabFilename then
			(
				setINISetting inipath "settings" "importmab" mabFilename

				set coordsys world

				local loader = MAB_Loader()
				loader.Open mabFilename
				loader.LoadModel()
				loader.Close()

				max views redraw
			)
		)
	)

	on importMAC pressed do
	(
		inipath = scriptsPath + "settings.ini"
		filename = getINISetting inipath "settings" "importmac"
	    macFilename = getOpenFileName caption:"Select MAC file" \
					filename:filename \
					types:"EDF MAC (*.mac)|*.mac|All Files (*.*)|*.*|"

		if macFilename != undefined then
		(
			if DoesFileReallyExist macFilename then
			(
				setINISetting inipath "settings" "importmac" macFilename

				set coordsys world

				local loader = MAC_Loader()
				loader.Open macFilename
				loader.LoadModel()
				loader.Close()

				max views redraw
			)
		)
	)

	on importMDB pressed do
	(
		inipath = scriptsPath + "/settings.ini"
		filename = getINISetting inipath "settings" "importmdb"
	    mdbFilename = getOpenFileName caption:"Select MDB file" \
					filename:filename \
					types:"EDF MDB (*.mdb)|*.mdb|All Files (*.*)|*.*|"

		if mdbFilename != undefined then
		(
			if DoesFileReallyExist mdbFilename then
			(
				setINISetting inipath "settings" "importmdb" mdbFilename

				set coordsys world

				local loader = MDB_Loader()
				loader.Open mdbFilename
				loader.LoadModel()
				loader.Close()

				max views redraw
			)
		)
	)

	on importMDBtxt pressed do
	(
		inipath = scriptsPath + "settings.ini"
		filename = getINISetting inipath "settings" "importmdb"
	    mdbFilename = getOpenFileName caption:"Select MDB file" \
					filename:filename \
					types:"EDF model (*.xml)|*.xml|All Files (*.*)|*.*|"

		if mdbFilename != undefined then
		(
			if DoesFileReallyExist mdbFilename then
			(
				setINISetting inipath "settings" "importmdb" mdbFilename

				set coordsys world

				local loader = MDB_TXTLoader()
				loader.Open mdbFilename
				loader.LoadModel()
				loader.Close()

				max views redraw
			)
		)
	)

	-- output

	on OutputCoord pressed do
	(
		inipath = scriptsPath + "settings.ini"
		filename = getINISetting inipath "settings" "outcoordinate"
		outputCoordFilename = getSaveFileName caption:"Coordinate File" \
							filename:filename \
							types:"Text (*.txt)|*.txt|All Files (*.*)|*.*|"

		if outputCoordFilename != undefined then
		(
			local exporter = EDFCoordinate_Out()

			exporter.EDFCoordinate outputCoordFilename
			
		)
	)

	on exportMDBtxt pressed do
	(
		inipath = scriptsPath + "settings.ini"
		filename = getINISetting inipath "settings" "exportmdb"
		mdbFilename = getSaveFileName caption:"Select MDB file" \
							filename:filename \
							types:"Text (*.xml)|*.xml|All Files (*.*)|*.*|"

		if mdbFilename != undefined then
		(
			local exporter = MDB_TXTExporter()

			exporter.ExportModel mdbFilename
		)
	)
	
	on OutputMABPoint pressed do
	(
		local processor = EDFMABPoint()

		processor.MABPoint()
	)

	-- other
	on FixVertUV pressed do
	(
		local processor = EDFFixVertUV()

		processor.FixUV()
	)
)
