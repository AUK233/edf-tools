fileIn "EDFutility.ms"

struct MAB_Header
(
	endian,	-- "MAB\0"
	unk1,
	unk2,
	bonegnum,	-- 0x2	
	animgnum,	-- 0x2	
	unk3,
	bonegoffset,
	animgoffset,
	unk4,
	unk5,

	fn LoadFromStream bstream=
	(
		endian = ReadLong bstream #unsigned
		unk1 = ReadLong bstream #unsigned
		unk2 = ReadLong bstream #unsigned
		bonegnum = ReadShort bstream #unsigned
		animgnum = ReadShort bstream #unsigned
		unk3 = ReadLong bstream #unsigned
		bonegoffset = ReadLong bstream #unsigned
		animgoffset = ReadLong bstream #unsigned
		unk4 = ReadLong bstream #unsigned
		unk5 = ReadLong bstream #unsigned
	)
)

struct MAB_bonegroup
(
	unk,
	num,
	offset,

	fn LoadFromStream bstream=
	(
		unk = ReadShort bstream #unsigned
		num = ReadShort bstream #unsigned
		offset = ReadLong bstream #unsigned
	)
)

struct MAB_BAgroup
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

struct MAB_Loader
(
	bstream,	--bitsream
	header,		--file header
	bonegroup=#(),
	BAgroup=#(),
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
		header = MAB_Header()
		header.LoadFromStream bstream

		if header.bonegnum > 0 then
		(
			fseek bstream header.bonegoffset #seek_set
			local bgtemp = MAB_bonegroup()
			for i = 1 to header.bonegnum do
			(
				bgtemp.LoadFromStream bstream
				append bonegroup (copy bgtemp)

				format "groupid: %\n" bonegroup[i].unk
				format "bonenum: %\n" bonegroup[i].num
			)

			format "groupnum: %\n" bonegroup.count

			for i = 1 to bonegroup.count do
			(
				format "debug: %\n" i
				format "debugnum: %\n" bonegroup[i].num
				if bonegroup[i].num > 0 then
				(
					fseek bstream bonegroup[i].offset #seek_set
					local bagtemp = MAB_BAgroup()

					for j = 1 to bonegroup[i].num do
					(
						bagtemp.LoadFromStream bstream
						append BAgroup (copy bagtemp)

						format "bonepos: %\n" BAgroup[j].pos
					)

					format "bonenum: %\n" BAgroup.count

					for j = 1 to BAgroup.count do
					(
						if BAgroup[j].Pos > 0 then
						(
							fseek bstream BAgroup[j].Pos #seek_set
							zpos =#()
							xpos =#()
							ypos =#()

							zpos = ReadFloat bstream
							xpos = ReadFloat bstream
							ypos = ReadFloat bstream
							-- wpos = ReadFloat bstream
							obj = sphere name:(BAgroup[j].name) pos:[xpos,ypos,zpos] radius:0.1 segs:4 wirecolor:[0,128,0]
							format "x: %, y: %, z: %\n" xpos ypos zpos
						)
					)
				)
			)
		)
	)
)
