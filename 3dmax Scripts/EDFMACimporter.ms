fileIn "EDFutility.ms"

struct MAC_Header
(
	-- 36 bytes
	i1,
	i2,
	i3,
	i4,
	i5,
	MARC,
	num,
	i8,
	i9,
	pos,

	fn LoadFromStream bstream=
	(
		i1 = ReadLong bstream #unsigned
		i2 = ReadLong bstream #unsigned
		i3 = ReadLong bstream #unsigned
		i4 = ReadLong bstream #unsigned
		i5 = ReadLong bstream #unsigned
		pos = ftell bstream
		MARC = ReadLong bstream #unsigned
		num = ReadLong bstream #unsigned
		i8 = ReadLong bstream #unsigned
		i9 = ReadLong bstream #unsigned
	)
)

struct MAC_MarcTable
(
	offset,
	i1,
	i2,
	i3,
	i4,
	i5,

	fn LoadFromStream bstream=
	(
		i1 = ReadLong bstream #unsigned
		offset = ReadLong bstream #unsigned
		i2 = ReadLong bstream #unsigned
		i3 = ReadLong bstream #unsigned
		i4 = ReadLong bstream #unsigned
		i5 = ReadLong bstream #unsigned
	)
)

struct MAC_MAPB
(
	fourcc,
	i1,
	i2,
	i3,
	i4,
	i5,
	i6,
	i7,
	offset2,
	num2,
	isok,
	pos,

	fn LoadFromStream bstream=
	(
		pos = ftell bstream
		fourcc = ReadFixedString bstream 4 true
		if fourcc == "MAPB" then
		(
			i1 = ReadLong bstream #unsigned
			i2 = ReadLong bstream #unsigned
			i3 = ReadLong bstream #unsigned
			i4 = ReadLong bstream #unsigned
			i5 = ReadLong bstream #unsigned
			i6 = ReadLong bstream #unsigned
			i7 = ReadLong bstream #unsigned
			offset2 = ReadLong bstream #unsigned
			num2 = ReadLong bstream #unsigned
			isok = true
		)
		else
		(
			isok = false
			i1 = 0
			i2 = 0
			i3 = 0
			i4 = 0
			i5 = 0
			i6 = 0
			i7 = 0
			offset2 = 0
			num2 = 0
		)
	)
)

struct MAC_MAPB_2nd
(
	offset,
	px,
	py,
	pz,
	rx,
	ry,
	rz,

	fn LoadFromStream bstream=
	(
		offset = ReadLong bstream #signed
		py = ReadFloat bstream
		pz = ReadFloat bstream
		px = ReadFloat bstream
		ry = ReadFloat bstream
		rz = ReadFloat bstream
		rx = ReadFloat bstream

		fseek bstream 40 #seek_cur

		obj1 = point name:(offset as string) pos:[px,py,pz] axistripod:false box:true constantscreensize:true size:1 centermarker:false cross:false wirecolor:[0,255,0]
		obj2 = sphere name:(offset as string) pos:[px,py,pz] radius:2.5 segs:4 smooth:false wirecolor:[0,128,0]

		--obj = sphere name:(offset as string) radius:0.1 segs:4 wirecolor:[0,128,0]
		--move obj [px,py,pz]
	)
)

struct MAC_Loader
(
	bstream,	--bitsream
	header,		--file header
	MarcTable=#(),
	MAPB=#(),
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
		header = MAC_Header()
		header.LoadFromStream bstream

		if header.num > 0 then
		(
			fseek bstream (header.MARC + header.pos) #seek_set
			local marctemp = MAC_MarcTable()
			for i = 1 to header.num do
			(
				marctemp.LoadFromStream bstream
				append MarcTable (copy marctemp)
			)

			for i = 1 to MarcTable.count do
			(
				if MarcTable[i].offset > 0 then
				(
					fseek bstream MarcTable[i].offset #seek_set
					local mapbtemp = MAC_MAPB()

					mapbtemp.LoadFromStream bstream
					append MAPB (copy mapbtemp)
				)
			)

			for i = 1 to MAPB.count do
			( 
				if MAPB[i].isok == true then
				(
					if MAPB[i].num2 > 0 then
					(
						fseek bstream (MAPB[i].pos + MAPB[i].offset2) #seek_set
						format "offset2: %\n" MAPB[i].offset2
						
						for j = 1 to MAPB[i].num2 do
						(
							local mapb2temp = MAC_MAPB_2nd()
							mapb2temp.LoadFromStream bstream

							--format "x: %, y: %, z: %\n" xpos ypos zpos
						)
					)
				)
			)
		)
	)
)
