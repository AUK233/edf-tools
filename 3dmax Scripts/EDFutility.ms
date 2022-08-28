--fileIn "EDFutf16.ms"

fn GetWString bstream=
(
	local str = ""
	local isrun = true
	while isrun do
	(
		local tempchar1 = ReadByte bstream #unsigned
		local tempchar2 = ReadByte bstream #unsigned
		--tempchar = bit.swapBytes tempchar 1 2

		--format "name: %\n" (bit.intAsHex tempchar)
		local nonzero = tempchar1 + tempchar2
		if nonzero == 0 then
		(
			isrun = false
		)
		else
		(
			if tempchar2 == 0 then
			(
				str += bit.intAsChar tempchar1
			)
			else
			(
				str += ("\\x"+ toUpper((bit.intAsHex tempchar1) as string))
				str += ("\\x"+ toUpper((bit.intAsHex tempchar2) as string))
				--str += getUTF16 tempchar
			)
		)
	)
	str
)

fn ReadFixedString bstream fixedLen junk=
(
	local str = ""
	for i = 1 to fixedLen do
	(
		local ch = ReadByte bstream #unsigned
		--format "Read string: %\n" ch
		if junk == true then
		(
			str += bit.intAsChar ch
		)
		--str += bit.intAsChar ch
		else if junk == false then
		(
			if ch == 0 then 
			(
				--replace with a tab "\t"
				str += "\t"
			)
			else
			(
				str += bit.intAsChar ch
			)
		)
	)
	str
)
