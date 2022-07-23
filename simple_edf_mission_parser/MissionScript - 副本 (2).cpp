#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
#include <vector>
#include <codecvt>
#include <sstream>

#include <iostream>
#include <locale>
#include "util.h"
#include "JSONAMLParser.h"
#include "MissionScript.h"
#include "VMState.h"

void CMissionScript::LoadLanguage( std::wstring path, int id )
{
	languageFile[id] = std::make_unique<CJSONAMLParser>( path );
}

int CMissionScript::Read( const std::wstring& path )
{
	std::ifstream file( path + L".BVM", std::ios::binary | std::ios::ate);

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	if( size == -1 )
		return -1;

	std::vector<char> buffer(size);
	if (file.read(buffer.data(), size))
	{
		unsigned char seg[4];

		int position = 0;
		//position = 0x100;

		Read4BytesReversed( seg, buffer, position );

		bool validHeader = false;
		if( seg[0] == 0x42 && seg[1] == 0x56 && seg[2] == 0x4D && seg[3] == 0x20 )
			validHeader = true;
		else
		{
			Read4Bytes( seg, buffer, position );
			if( seg[0] == 0x42 && seg[1] == 0x56 && seg[2] == 0x4D && seg[3] == 0x20 )
				validHeader = true;
		}

		if( !validHeader )
		{
			std::wcout << L"BAD FILE\n";
			file.close();
			return -1;
		}

		output = std::wofstream("MISSION_Data.txt", std::ios::binary | std::ios::out | std::ios::ate );

		const unsigned long MaxCode = 0x10ffff;
		const std::codecvt_mode Mode = std::generate_header;
		std::locale utf16_locale(output.getloc(), new std::codecvt_utf16<wchar_t, MaxCode, Mode>);
		output.imbue(utf16_locale);

		//Get string array position
		position = 0x38;
		Read4Bytes( seg, buffer, position );
		m_iStringBufferOfs = GetIntFromChunk( seg );

		//Read variable names
		//Get variable name count
		position = 0x18;
		Read4Bytes( seg, buffer, position );
		int count = GetIntFromChunk(seg);
		//Get offset to variable name table
		position = 0x1c;
		Read4Bytes( seg, buffer, position );
		position = GetIntFromChunk(seg);

		//Read
		for( int i = 0; i < count; i++ )
		{
			//Read offset
			Read4Bytes( seg, buffer, position );
			int pos = GetIntFromChunk( seg );

			m_vecVarNames.push_back( ReadUnicode( buffer, pos ) );

			position += 4;
		}

		//Todo: Get better data from this.
		//Count of array 2
		position = 0x20;
		Read4Bytes( seg, buffer, position );
		count = GetIntFromChunk( seg );

		position = 0x30;
		Read4Bytes( seg, buffer, position );
		int unknownChunkStart = GetIntFromChunk( seg );

        //Determine initial values of all static variables
		//Big thanks to Souzooka for designing this system.
        std::wstring variable_data;
        VMState runtime_state = VMState(m_vecVarNames);
        runtime_state.interpret((unsigned char*)&(buffer.data()[unknownChunkStart]));
        runtime_state.printstatics(variable_data);

		//Reformat string to more ideal state:
		FindAndReplaceAll( variable_data, L"\n", L"\r\n" );

		//Get function data offset
		position = 0x34;
		Read4Bytes( seg, buffer, position );
		int functionDataOffset = GetIntFromChunk( seg );
		//functionDataOffset = buffer[functionDataOffset];

		//position of Function Array
		position = 0x24;
		Read4Bytes( seg, buffer, position );
		position = GetIntFromChunk( seg );


		m_iFunctionDataOfs = functionDataOffset;
		Read4Bytes( seg, buffer, position );
		m_iFunctionDataOfs += GetIntFromChunk( seg );

		//Read strings from array 2
		//position += 4;
		for( int i = 0; i < count; i++ )
		{
			//Read offset
			Read4Bytes( seg, buffer, position );
			int pos = GetIntFromChunk( seg );

			pos += functionDataOffset;

			pos++;
			Read4Bytes( seg, buffer, pos );

			seg[0] = 0xff;
			seg[1] = 0xff;

			int num2 = GetIntFromChunk( seg );

			m_vecFuncOffsets.push_back( (pos + num2) - 1 );

			//Read name:
			position += 4;

			Read4Bytes( seg, buffer, position );
			pos = GetIntFromChunk( seg );

			MissionFunction *func = new MissionFunction( );
			func->fnName = ReadUnicode( buffer, pos );

			position += 8;

			//Read4Bytes( seg, buffer, position );
			//num2 = GetIntFromChunk( seg );
			num2 = buffer[ position ];

			func->argCount = num2;
			func->doesReturn = buffer[ position + 1 ];

			m_vecFunctions.push_back( func );

			position += 4;
		}

		std::wcout << L"Dumping data...\n";

        output << variable_data + L"\r\n";
        std::wcout << variable_data + L"\r\n";

		//Loop through every function:
		for( int funcID = 0; funcID < m_vecFunctions.size(); funcID++ )
		{
			std::wstring strn = m_vecFunctions[funcID]->fnName;

			//Formating lol
			strn += L"( ";

			if( m_vecFunctions[funcID]->argCount > 0 )
			{
				for( int i = 0; i < m_vecFunctions[funcID]->argCount; i++ )
				{
					strn += L"arg" + ToString(i);
					strn += L", ";
				}
				//Erase last 2 chars.
				strn.pop_back();
				strn.pop_back();
			}

			strn += L" )\r\n{\r\n";

			output << strn;
			std::wcout << strn;

			//Read bytes.
			position = m_vecFuncOffsets[funcID];

			if( funcID + 1 < m_vecFunctions.size() )
				ReadFn( position, buffer, m_vecFunctions[funcID]->argCount, m_vecFuncOffsets[funcID + 1] );
			else
				ReadFn( position, buffer, m_vecFunctions[funcID]->argCount, m_iFunctionDataOfs );

			//Formating lol
			strn = L"}\r\n";

			output << strn;
			std::wcout << strn;
		}
		output.close();
	}

	file.close();
	return 1;
}

void CMissionScript::ReadFn( int position, std::vector<char> buffer, int numArgs, int nextFunctionStart )
{
	int ofs = 0;
	int numLocalVars = 0;

	//Read the function initialiser block and gain nessisary information
	char byte = buffer[position];
	if( byte == 0x5c )
	{
		ofs++;
		numLocalVars = buffer[position + ofs];
		numLocalVars -= numArgs;
	}

	ofs = position + (numArgs * 2) + 3;

	//Define the stack.
	std::vector< std::wstring > stack;
	unsigned char seg[4] = { 0, 0, 0, 0 };

	//Track number of "variables"
	int numVar = 0;
 
	bool isInIf = false;
	int ifEnd = 0;

	//Clear seg
	seg[0] = 0;
	seg[1] = 0;
	seg[2] = 0;
	seg[3] = 0;

	//Initialise stack with function arguements:
	//for( int i = 0; i < numArgs; i++ )
	//{
	//	stack.push_back( L"arg" + ToString(i) );
	//}

	//Start reading everything. Abort if hit end-of-function.
	while( ofs < m_iFunctionDataOfs && ofs < (nextFunctionStart - 4) )
	{
		if( ofs == ifEnd && isInIf )
		{
			std::wcout << L"	}\r\n";
			output << L"	}\r\n";

			isInIf = false;
		}
		unsigned char byte = buffer[ofs];
		unsigned int operationByte = byte & 0x3F;

		//Get size:
		int size = 0;
		if( byte > 0x40 )
			size = 1;
		if( byte > 0x80 )
			size = 2;
		if( byte > 0xc0 )
			size = 4;

		//Aurgh! This needs to be refactored!
		if( byte == 0x69 )//Call
		{
			std::wstring strn = L"	";

			seg[0] = 0xff;
			seg[1] = 0xff;
			seg[2] = 0xff;
			seg[3] = buffer[ofs + 1];

			int num = GetIntFromChunk( seg );

			num = ofs + num;

			bool found = false;
			bool ret = false;
			for( int i = 0; i < m_vecFuncOffsets.size( ); i++ )
			{
				if( m_vecFuncOffsets[i] == num )
				{
					found = true;

					strn = m_vecFunctions[i]->fnName + L"( ";

					ret = m_vecFunctions[i]->doesReturn;

					if( m_vecFunctions[i]->argCount > 0 )
					{
						for( int j = 0; j < m_vecFunctions[i]->argCount; j++ )
						{
							if( stack.size( ) > 0 )
							{
								strn += stack.back( );
								stack.pop_back( );
							}
							else
								strn += L"STACK_SIZE_ERROR";
							strn += L", ";
						}
						//Erase last 2 chars.
						strn.pop_back( );
						strn.pop_back( );
					}

					strn += L" )";
				}
			}
			if( !found )
			{
				strn = L"Jump( " + std::to_wstring( (uint64_t)GetIntFromChunk( seg ) ) + L" )";
			}
			if( !ret )
			{
				strn += L"\r\n";

				std::wcout << L"	" + strn;
				output << L"	" + strn;
			}
			else
			{
				stack.push_back( strn );
			}

			ofs += 2;
			continue;
		}
		if( byte == 0xa9 )//Call
		{
			std::wstring strn =	L"	";
			
			seg[0] = 0xff;
			seg[1] = 0xff;
			seg[2] = buffer[ofs +2];
			seg[3] = buffer[ofs +1];

			int num = GetIntFromChunk( seg );

			num = ofs + num;

			bool found = false;
			bool ret = false;
			for( int i = 0; i < m_vecFuncOffsets.size(); i++ )
			{
				if( m_vecFuncOffsets[i] == num )
				{
					found = true;

					strn = m_vecFunctions[i]->fnName + L"( ";

					ret = m_vecFunctions[i]->doesReturn;

					if( m_vecFunctions[i]->argCount > 0 )
					{
						for( int j = 0; j < m_vecFunctions[i]->argCount; j++ )
						{
							if( stack.size() > 0 )
							{
								strn += stack.back();
								stack.pop_back();
							}
							else
								strn += L"STACK_SIZE_ERROR";
							strn += L", ";
						}
						//Erase last 2 chars.
						strn.pop_back();
						strn.pop_back();
					}

					strn+= L" )";
				}
			}
			if( !found )
			{
				strn = L"Jump( " + std::to_wstring( (uint64_t)GetIntFromChunk( seg ) ) + L" )";
			}
			if( !ret )
			{
				strn += L"\r\n";

				std::wcout << L"	" + strn;
				output <<  L"	" + strn;
			}
			else
			{
				stack.push_back( strn );
			}

			ofs += 3;
			continue;
		}

		//Override size:
		switch( operationByte )
		{
		case 0x4:
			size = 1;
			break;
		case 0x6:
			size = 1;
			break;
		};

		//Clear seg
		seg[0] = 0;
		seg[1] = 0;
		seg[2] = 0;
		seg[3] = 0;

		//Read based on size:
		for( int i = 0; i < size; i++ )
		{
			ofs++;
			seg[3-i] = buffer[ofs];
		}

		//Handle the operator byte.
		switch( operationByte )
		{
		case 0x1: //Copy value to BVM RAS. [stack - 1] = index, [stack] = value. Arithmetic type specifier indicates source type (LSB) and destination type (MSB). Remove index from stack.
		{
			if( stack.size() == 0 )
				stack.push_back( L"STACK_ERROR" );

			std::wstring val = stack.back();
			stack.pop_back();

			if( stack.size() == 0 )
				stack.push_back( L"STACK_ERROR" );

			std::wstring result;

			//Check for valid number:
			/*
			int num;
			bool valid = true;
			try
			{
				num = stoi( stack.back() );
			}
			catch (const std::exception& e)
			{
				valid = false;
			}
			if( num < m_vecVarNames.size() && valid )
			{
				result = m_vecVarNames[num] + L" = " + val;
			}
			else
			*/
			{
				result = L"RAS[ " + stack.back() + L" ] = " + val;
			}

			stack.pop_back();
			stack.push_back( result );

			std::wcout << L"	" + result + L"\r\n";
			output << L"	" + result + L"\r\n";
		}
		break;

		case 0x2: //Pop stack.
			if( stack.size() > 0 )
				stack.pop_back();
		break;

		case 0x03: //Duplicate stack x times
		{
			int num = GetIntFromChunk( seg );
			if( stack.size() > 0 )
			{
				std::wstring strn = stack.back();
				for( int i = 0; i < num; i++ )
				{
					stack.push_back( strn );
				}
			}
		}
		break;

		case 0x04: //Add
		{
			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring strn1 = stack.back();
			stack.pop_back();

			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );
			
			std::wstring out = stack.back() + L" + " + strn1;
			stack.pop_back( );
			stack.push_back( out );
		}
		break;
		case 0x05: //Subtract
		{
			//Failsafe
			if( stack.size() == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring strn1 = stack.back();
			stack.pop_back();

			//Failsafe
			if( stack.size() == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring out = stack.back() + L" - " + strn1;
			stack.push_back( out );
		}
		break;

		case 0x06: //Multiply
		{
			//Failsafe
			if( stack.size() == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring strn1 = stack.back();
			stack.pop_back();

			//Failsafe
			if( stack.size() == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring out = stack.back() + L" * " + strn1;
			stack.push_back( out );
		}
		break;

		case 0x07: //Divide
		{
			//Failsafe
			if( stack.size() == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring strn1 = stack.back();
			stack.pop_back();

			//Failsafe
			if( stack.size() == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring out = stack.back() + L" / " + strn1;
			stack.push_back( out );
		}
		break;

		case 0x08: //modulo
		{
			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring strn1 = stack.back();
			stack.pop_back();

			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring out = L"mod( " + stack.back() + L", " + strn1 + L" )";
			stack.push_back( out );
		}
		break;

		case 0x09: //Negate value at stack.
		{
			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring out = L"negate( " + stack.back() + L" )";
			stack.pop_back();
			stack.push_back( out );
		}
		break;

		case 0x0a: //Incrmement value at stack.
		{
			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring out = stack.back() + L"++";
			stack.pop_back();
			stack.push_back( out );
		}
		break;

		case 0x14: //Get from BVM RAS
		{
			int num = GetIntFromChunk( seg );
			if( num < m_vecVarNames.size() )
			{
				stack.push_back( m_vecVarNames[num ] );
			}
			else
			{
				std::wstring strn = L"RAS[ " + ToString(GetIntFromChunk( seg )) + L" ]";
				stack.push_back( strn );
			}
		}
		break;

		case 0x15: //Read x Bytes (Likely as a number)
		{
			if( size == 0 ) //Simply print false.
			{
				stack.push_back( L"false" );
			}
			else if( byte == 0xD5 ) //Override for float. Probably not exatcly needed, but every instance of D5 I have sene has been a float.
			{
				//Float is actualy the wrong way around!
				char reversedSeg[4];
				reversedSeg[0] = seg[3];
				reversedSeg[1] = seg[2];
				reversedSeg[2] = seg[1];
				reversedSeg[3] = seg[0];

				float f;
				memcpy(&f, &reversedSeg, sizeof(f));
				stack.push_back( std::to_wstring( (long double)f ) );
			}
			else
				stack.push_back( std::to_wstring( (uint64_t)GetIntFromChunk( seg ) ) );
		}
		break;

		case 0x16: //Pop value from stack and move it to BVM RAS, next <size bytes> absolute index
		{
			if( stack.size() > 0 )
			{
				std::wstring strn = stack.back();
				std::wstring out = L"	";
				stack.pop_back();
				int num = GetIntFromChunk( seg );
				if( num < m_vecVarNames.size() )
				{
					out += m_vecVarNames[num] + L" = " + strn + L"\r\n";
					std::wcout << out;
					output << out;
				}
				else
				{
					out += L"RAS[ " + ToString( num ) + L" ] = " + strn + L"\r\n";
					std::wcout << out;
					output << out;
				}
			}
		}
		case 0x17: //Retrieve value from BVM RAS and push it to BVM stack, relative index
		{
			int varNumber = GetIntFromChunk( seg );
			if( varNumber <= numArgs )
				stack.push_back( L"arg" + ToString( varNumber - 1 ) );
			else
				stack.push_back( L"localVar" + ToString( varNumber - 1 ) );
		}
		break;

		case 0x18: //Push stored relative BVM RAS index + next <size bytes> to stack
		{
			stack.push_back( L"localVar." + ToString( GetIntFromChunk( seg ) - 1 ) );
		}
		break;

		case 0x1B: //Increment index that BVM RAS is accessed by the next <size> bytes
		{
			std::wcout << L"	relIndex += " + ToString( GetIntFromChunk( seg ) ) + L"\r\n";
			output << L"	relIndex += " + ToString( GetIntFromChunk( seg ) ) + L"\r\n";
		}
		break;

		case 0x1C: //Decrement index that BVM RAS is accessed by the next <size> bytes
		{
			std::wcout << L"	relIndex -= " + ToString( GetIntFromChunk( seg ) ) + L"\r\n";
			output << L"	relIndex -= " + ToString( GetIntFromChunk( seg ) ) + L"\r\n";
		}
		break;

		case 0x33: //Pushes true somewhere?
			stack.push_back( L"true" );
			break;

		case 0x12: //Convert top value from float to int
		{
			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring strn = L"(int)" + stack.back();
			stack.pop_back();
			stack.push_back( strn );
		}
		break;

		case 0x13: //Convert top value from int to float
		{
			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring strn = L"(float)" + stack.back();
			stack.pop_back();
			stack.push_back( strn );
		}
		break;

		case 0x1a: //String
		{
			int strTableOffset = GetIntFromChunk( seg );
			std::wstring strn = ReadUnicode( buffer, m_iStringBufferOfs + strTableOffset );
			stack.push_back( L"\"" + strn + L"\"" );
		}
		break;

		case 0x20: //x < y
		{
			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring y = stack.back();
			stack.pop_back();

			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring x = stack.back();
			stack.pop_back();

			stack.push_back( x + L" < " + y );
		}
		break;

		case 0x21: //x <= y
		{
			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring y = stack.back();
			stack.pop_back();

			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring x = stack.back();
			stack.pop_back();

			stack.push_back( x + L" <= " + y );
		}
		break;

		case 0x22: //x == y
		{
			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring y = stack.back();
			stack.pop_back();

			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring x = stack.back();
			stack.pop_back();

			stack.push_back( x + L" == " + y );
		}
		break;

		case 0x23: //x != y
		{
			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring y = stack.back();
			stack.pop_back();

			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring x = stack.back();
			stack.pop_back();

			stack.push_back( x + L" != " + y );
		}
		break;

		case 0x24: //x >= y
		{
			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring y = stack.back();
			stack.pop_back();

			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring x = stack.back();
			stack.pop_back();

			stack.push_back( x + L" >= " + y );
		}
		break;

		case 0x25: //x > y
		{
			//Failsafe
			if( stack.size() == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring y = stack.back();
			stack.pop_back();

			//Failsafe
			if( stack.size() == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring x = stack.back();
			stack.pop_back();

			stack.push_back( x + L" > " + y );
		}
		break;

		case 0x26: //jump if stack == 0
		{
			//Failsafe
			if( stack.size() == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring str = L"	if( " + stack.back() + L" )\r\n	{\r\n";
			stack.pop_back();

			isInIf = true;
			ifEnd = GetIntFromChunk( seg ) + ofs - size;

			std::wcout << str;
			output << str;
		}
		break;

		case 0x27: //jump if stack != 0
		{
			//Failsafe
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_SIZE_ERROR" );

			std::wstring str = L"	if( !(" + stack.back( ) + L") )\r\n	{\r\n";
			stack.pop_back( );

			isInIf = true;
			ifEnd = GetIntFromChunk( seg ) + ofs - size;

			std::wcout << str;
			output << str;
		}
		break;

		case 0x28: //Jump
		{
			std::wstring str = L"}\r\n";

			for( int i = 0; i < 4 - size; i++ )
			{
				seg[i] = 0xff;
			}

			int num = GetIntFromChunk( seg );

			if( (ofs-size) + num == (position - 4) )
			{
				std::wstring str = L"	return;\r\n";

				std::wcout << str;
				output << str;
				break;
			}

			std::wcout << str;
			output << str;
		}
		break;
		case 0x32: //Gutted print function:
		{
			if( stack.size( ) == 0 )
				stack.push_back( L"STACK_EMPTY" );

			std::wstring strn = L"Debug: " + stack.back();
			std::wcout << strn; stack.pop_back();
		}
		break;

		case 0x36: //Push to BVM RAS
		{
			if( stack.size() == 0 )
				stack.push_back( L"var" );

			std::wstring value1 = stack.back();
			stack.pop_back();

			if( stack.size() == 0 )
				stack.push_back( L"var" );

			std::wstring value2 = stack.back( );
			stack.pop_back( );

			//Get var name?
			std::wstring strn;

			//strn = L"	RAS[ " + stack.back() + L" ] = " + value1 + L"\r\n";
			std::wstring tokenisedStrn = value2.c_str( );
			strn = SimpleTokenise( tokenisedStrn, L'.' );
			if( strn == L"localVar" )
			{
				strn = L"	localVar" + tokenisedStrn + L" = " + value1 + L"\r\n";
			}
			else
			{
				//Check for valid number:
				int num;
				bool valid = true;
				try
				{
					num = stoi( value2 );
				}
				catch( const std::exception& e )
				{
					valid = false;
				}
				if( num < m_vecVarNames.size( ) && valid )
					strn = L"	" + m_vecVarNames[num] + L" = " + value1 + L"\r\n";
			}

			//stack.pop_back();
			std::wcout << strn;
			output << strn;
		}
		break;

			case 0x2c://Exectute:
			{
				//output << L"	0x2C\r\n";
				//std::wcout << L"	0x2C\r\n";

				std::wstringstream wss;
				wss << std::hex << GetIntFromChunk( seg );
				std::wstring compare(L"0x" + wss.str( ));

				JSONAMLNode *node = languageFile[0]->FindNode( L"representations", compare );
				if( node )
				{
					std::wstring name = node->GetValue( L"name" );
					if( name.size( ) == 0 )
					{
						name = L"2C( " + compare + L", ";
					}
					else
					{
						name += L"( ";
					}

					JSONAMLValueArr *args = languageFile[0]->GetValueAsArrNode( node, L"arguments" );

					int argC = 0;
					if( args )
						argC = args->nodes.size( );

					if( argC > 0 )
					{
						for( int i = 0; i < argC; ++i )
						{
							if( stack.size( ) == 0 )
								stack.push_back( L"STACK_SIZE_ERROR" );

							name += stack.back( );
							stack.pop_back( );

							if( !( i == argC - 1 ) )
								name += L", ";
						}
					}
					else if( name.size() == 0 )
					{
						name.pop_back( );
						name.pop_back( );
					}

					name += L" )";

					JSONAMLValueSubnode *retVals = (JSONAMLValueSubnode *)( node->values[L"returnValue"].get( ) );

					if( !retVals || retVals->node->values.size() == 0 || retVals->node->GetValue( L"type" ) == L"void" )
					{
						std::wcout << L"	" + name + L";\r\n";
						output << L"	" + name + L";\r\n";
					}
					else
					{
						stack.push_back( name );
					}
				}
				else
				{
					std::wstring name = L"2C( 0x" + wss.str( ) + L" )";

					std::wcout << L"	" + name + L"\r\n";
					output << L"	" + name + L"\r\n";
				}
			}
			break;
			case 0x2D:
			{
				std::wstringstream wss;
				wss << std::hex << GetIntFromChunk( seg );
				std::wstring compare( L"0x" + wss.str( ) );

				JSONAMLNode *node = languageFile[1]->FindNode( L"representations", compare );
				if( node )
				{
					std::wstring name = node->GetValue( L"name" );
					if( name.size( ) == 0 )
					{
						name = L"2D( " + compare + L", ";
					}
					else
					{
						name += L"( ";
					}

					JSONAMLValueArr *args = languageFile[1]->GetValueAsArrNode( node, L"arguments" );

					int argC = 0;
					if( args )
						argC = args->nodes.size( );

					if( argC > 0 )
					{
						for( int i = 0; i < argC; ++i )
						{
							if( stack.size( ) == 0 )
								stack.push_back( L"STACK_SIZE_ERROR" );

							name += stack.back( );
							stack.pop_back( );

							if( !( i == argC - 1 ) )
								name += L", ";
						}
					}
					else if( name.size( ) == 0 )
					{
						name.pop_back( );
						name.pop_back( );
					}

					name += L" )";

					JSONAMLValueSubnode *retVals = (JSONAMLValueSubnode *)( node->values[L"returnValue"].get( ) );

					if( !retVals || retVals->node->values.size( ) == 0 || retVals->node->GetValue( L"type" ) == L"void" )
					{
						std::wcout << L"	" + name + L";\r\n";
						output << L"	" + name + L";\r\n";
					}
					else
					{
						stack.push_back( name );
					}
				}
				else
				{
					std::wstring name = L"2D( 0x" + wss.str( ) + L" )";

					std::wcout << L"	" + name + L"\r\n";
					output << L"	" + name + L"\r\n";
				}
			}
			break;
			case 0x2E:
			{
				output << L"	0x2E\r\n";
				std::wcout << L"	0x2E\r\n";
			}
			break;
			case 0x2F:
			{
				output << L"	0x2F\r\n";
				std::wcout << L"	0x2F\r\n";
			}
			break;

			case 0x30: //Exit?
				//stack.clear();
				//std::wcout << L"EXIT\r\n";
				//output << L"EXIT\r\n";
			break;
		}
		ofs++;
	}
}

void PushStringToVector( std::string strn, std::vector< char > *bytes )
{
	for( int i = 0; i < strn.size(); i++ )
	{
		bytes->push_back( strn[i] );
	}
}

//BVM header
BVMHeader::BVMHeader()
{
	headerString = "BVM ";

	Padding1 = (char*)malloc( sizeof( char )*8 );
	for( int i = 0; i < 8; i++ )
	{
		Padding1[i] = 0x00;
	}

	Unknown1 = (char*)malloc( sizeof( char )*5 );
	char unknown1[5] = { 0x38, 0x1, 0x15, 0x1, 0x2 };

	for( int i = 0; i < 5; i++ )
	{
		Unknown1[i] = unknown1[i];
	}

	Unknown2 = (char*)malloc( sizeof( char )*8 );
	char unknown2[8] = { 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00 };

	for( int i = 0; i < 8; i++ )
	{
		Unknown2[i] = unknown2[i];
	}
};

BVMHeader::~BVMHeader()
{
	free( Padding1 );
	free( Unknown1 );
	free( Unknown2 );

	headerString.clear();
}

std::vector< char > BVMHeader::GenerateBytes()
{
	char *seg;
	std::vector< char > bytes;

	//"BVM ".
	PushStringToVector( headerString, &bytes );

	//50 (P) 0 0 0
	bytes.push_back( 0x50 );
	for( int i = 0; i < 3; i++ )
	{
		bytes.push_back( 0x00 );
	}
	//free( seg );

	for( int i = 0; i < 8; i++ )
	{
		bytes.push_back( Padding1[i] );
	}

	for( int i = 0; i < 5; i++ )
	{
		bytes.push_back( Unknown1[i] );
	}

	bytes.push_back( 0x0 );
	bytes.push_back( 0x0 );
	bytes.push_back( 0x0 );

	//Num vars
	seg = IntToBytes( varArrSize );
	for( int i = 0; i < 4; i++ )
	{
		bytes.push_back( seg[i] );
	}
	free( seg );

	//Offset to vars
	seg = IntToBytes( varArrPtr );
	for( int i = 0; i < 4; i++ )
	{
		bytes.push_back( seg[i] );
	}
	free( seg );

	//Num functions
	seg = IntToBytes( fnArrSize );
	for( int i = 0; i < 4; i++ )
	{
		bytes.push_back( seg[i] );
	}
	free( seg );

	//Offset to functions
	seg = IntToBytes( fnArrPtr );
	for( int i = 0; i < 4; i++ )
	{
		bytes.push_back( seg[i] );
	}
	free( seg );

	//Unknown 2
	for( int i = 0; i < 8; i++ )
	{
		bytes.push_back( Unknown2[i] );
	}

	//Pointer to default values
	seg = IntToBytes( fnArrPtr + (fnArrSize*16) );
	for( int i = 0; i < 4; i++ )
	{
		bytes.push_back( seg[i] );
	}
	free( seg );

	int sizeOfVarInitialisers = 1;

	//Pointer to functions start
	seg = IntToBytes( fnArrPtr + (fnArrSize*16) + sizeOfVarInitialisers ); //TODO: Make this more valid lol
	for( int i = 0; i < 4; i++ )
	{
		bytes.push_back( seg[i] );
	}
	free( seg );

	//Pointer to stringtable
	seg = IntToBytes( strnPtr );
	for( int i = 0; i < 4; i++ )
	{
		bytes.push_back( seg[i] );
	}
	free( seg );

	//Pointer to string refering to BVM type (?)
	seg = IntToBytes( strnPtr );
	for( int i = 0; i < 4; i++ )
	{
		bytes.push_back( seg[i] );
	}
	free( seg );

	//padding
	for( int i = 0; i < 4; i++ )
	{
		bytes.push_back( 0x00 );
	}

	//Pointer to some kind of padding bytes?
	seg = IntToBytes( 0 );
	for( int i = 0; i < 4; i++ )
	{
		bytes.push_back( seg[i] );
	}
	free( seg );

	//padding
	for( int i = 0; i < 4; i++ )
	{
		bytes.push_back( 0x00 );
	}

	//pointer to EOF
	seg = IntToBytes( 0 );
	for( int i = 0; i < 4; i++ )
	{
		bytes.push_back( seg[i] );
	}
	free( seg );

	return bytes;
}

//Handle preproccessor information
void CMissionScript::Preproccesser( std::wstring missionSourceRaw, std::wstring &missionSourceProccessed )
{
	std::wcout << L"Running preproccessor...\n";

	//Handle preproccessor stuff
	std::wstring preProccesserBuffer;
	bool isPreProccessor = false;
	int preproccessorInstructionType = -1;
	for( int i = 0; i < missionSourceRaw.size( ); i++ )
	{
		//Handle scans for preproccessor instructions
		if( missionSourceRaw[i] == L'#' )
		{
			isPreProccessor = true;
			continue;
		}
		if( isPreProccessor )
		{
			//If we hit a newline or semicolon, proccess this
			if( missionSourceRaw[i] == L'\n' || missionSourceRaw[i] == L';' )
			{
				//Define
				if( preproccessorInstructionType == 0 )
				{
					m_vecReplaceStrings.push_back( preProccesserBuffer );
					preProccesserBuffer.clear( );
				}
				//Include
				if( preproccessorInstructionType == 1 )
				{
					std::wcout << preProccesserBuffer + L"\n";
					std::wstring includeFile = ReadFile( preProccesserBuffer.c_str( ) );
					FindAndReplaceAll( includeFile, L"\r\n", L"\n" );
					std::wstring includeFileProccessed = includeFile;
					Preproccesser( includeFile, includeFileProccessed );
					missionSourceProccessed.insert( i, includeFileProccessed );
					preProccesserBuffer.clear( );
				}

				isPreProccessor = false;
				preproccessorInstructionType = -1;
				continue;
			}
			//A space means we move to the next phase
			if( missionSourceRaw[i] == L' ' )
			{
				if( preproccessorInstructionType == 0 )
				{
					m_vecReplaceStrings.push_back( preProccesserBuffer );
				}
				else
				{
					if( preProccesserBuffer == L"define" )
						preproccessorInstructionType = 0;
					if( preProccesserBuffer == L"include" )
					{
						preproccessorInstructionType = 1;
						std::wcout << L"Including file: ";
					}
				}

				preProccesserBuffer.clear( );
			}
			else
			{
				preProccesserBuffer += missionSourceRaw[i];
			}
			continue;
		}
	}
}

#define FLAG_VERBOSE 1
#define FLAG_CREATE_FOLDER 2

//Write a BVM file
void CMissionScript::Write( const std::wstring& path, int flags )
{
	//Output file
	std::ofstream newMission;
	if( flags & FLAG_CREATE_FOLDER )
	{
		std::wstring folderName;
		std::wstring oldFolder;
		const size_t last_slash_idx = path.rfind( L'\\' );
		if( std::string::npos != last_slash_idx )
		{
			folderName = path.substr( last_slash_idx + 1, path.size( ) - last_slash_idx );
			oldFolder = path.substr( 0, last_slash_idx );
		}

		CreateDirectoryExW( oldFolder.c_str(), path.c_str(), NULL );

		newMission = std::ofstream( path + L"\\" + L"Mission" + L".bvm", std::ios::binary | std::ios::out | std::ios::ate );
	}
	else
		newMission = std::ofstream( path + L".bvm", std::ios::binary | std::ios::out | std::ios::ate );

	//newMission.close( );
	//return;

	//Read input file and prepare function compilation
	std::wstring sourcePath = path + L".txt";
	std::wstring missionSourceRaw = ReadFile( sourcePath.c_str() );

	//Verbose
	std::wcout << L"Compiling file: " + sourcePath + L"\n";

	FindAndReplaceAll( missionSourceRaw, L"\r\n", L"\n" );

	std::wstring missionSourceProccessed = missionSourceRaw;
	
	Preproccesser( missionSourceRaw, missionSourceProccessed );

	//Parse source file into functions
	std::wstring fnBuffer;
	int bracketDepth = 0;
	for( int i = 0; i < missionSourceProccessed.size( ); i++ )
	{
		//Ignore certain parts:
		if( missionSourceProccessed[i] == L'#' )
		{
			while( missionSourceProccessed[i] != L'\n' && missionSourceProccessed[i] != L';' )
			{
				i++;
			}
		}
		//Comments
		if( missionSourceProccessed[i] == L'/' && missionSourceProccessed[i+1] == L'/' )
		{
			while( missionSourceProccessed[i] != L'\n' )
			{
				i++;
			}
		}

		//Test for 'static' variables:
		if( missionSourceProccessed[i] == L' ' )
		{
			std::wstring varBuf = fnBuffer;
			FindAndReplaceAll( varBuf, L"\n", L"" );
			if( varBuf.front( ) == 0xfeff )
				varBuf.erase( 0, 1 );
			if( varBuf == L"static" )
			{
				fnBuffer.clear( );

				//Get static variable string
				while( missionSourceProccessed[i] != L';' && missionSourceProccessed[i] != L'\n' )
				{
					fnBuffer += missionSourceProccessed[i];
					i++;
				}

				//Parse "static variable"
				wchar_t *token = wcstok( &fnBuffer[0], L" " );

				//var name
				token = wcstok( NULL, L" " );
				m_vecVarNames.push_back( token );

				fnBuffer.clear( );
				i++;
			}
			varBuf.clear( );
		}

		//Parse function and push to array
		if( missionSourceProccessed[i] == L'{' )
			bracketDepth++;
		if( missionSourceProccessed[i] == L'}' )
		{
			bracketDepth--;
			if( bracketDepth == 0 )
			{
				fnBuffer += missionSourceProccessed[i];

				//operate on strings using list strings to replace ( #define stuff )
				for( int j = 0; j < m_vecReplaceStrings.size( ); j += 2 )
				{
					FindAndReplaceAll( fnBuffer, m_vecReplaceStrings[j], m_vecReplaceStrings[j+1] );
				}

				m_vecFunctions.push_back( new MissionFunction( fnBuffer, this ) );

				fnBuffer.clear( );
				continue;
			}
		}

		fnBuffer += missionSourceProccessed[i];
	}

	//Init header
	header = new BVMHeader( );

	//Number of bytes that will be generated to automatically set up varables
	int sizeOfVarInitialisers = 1;

	header->varArrSize = m_vecVarNames.size();
	header->varArrPtr = 0x50;

	//Parse functions and generate function bytes
	std::vector< char > Fnbytes;

	//Populate fn names. Used to local calls
	for( int i = 0; i < m_vecFunctions.size( ); i++ )
	{
		m_vecFunctions[i]->GetName( );
	}

	//Todo: Autogenerate this
	header->fnArrSize = m_vecFunctions.size( );
	header->fnArrPtr = header->varArrPtr + (header->varArrSize * 4);

	int fnDataSize = header->fnArrSize * 16;
	int startOfFnData = header->fnArrPtr + fnDataSize + sizeOfVarInitialisers;
	
	m_iFunctionDataOfs = startOfFnData;

	//Compile functions
	for( int i = 0; i < m_vecFunctions.size( ); i++ )
	{
		m_vecFunctions[i]->Compile( );

		m_vecFuncOffsets.push_back( startOfFnData + ( Fnbytes.size( ) + 4 ) );

		//Add to fnBytes list
		Fnbytes.insert( Fnbytes.end( ), m_vecFunctions[i]->bytes.begin( ), m_vecFunctions[i]->bytes.end( ) );
	}

	//Patch instructions:
	for( int i = 0; i < m_vecFunctions.size( ); i++ )
	{
		for( int j = 0; j < m_vecFunctions[i]->patchBytePos.size(); j++ )
		{
			int ofs = ( m_vecFuncOffsets[i] - startOfFnData ) + m_vecFunctions[i]->patchBytePos[j];
			int start = ofs - 4;

			int fnOfs;

			for( int fnID = 0; fnID < m_vecFunctions.size( ); fnID++ )
			{
				if( m_vecFunctions[i]->patchFuncName[j] == m_vecFunctions[fnID]->fnName )
				{
					fnOfs = (m_vecFuncOffsets[fnID] - startOfFnData) - start;
					break;
				}
			}

			char *seg = IntToBytes( fnOfs );

			Fnbytes[start + 1] = seg[0];
			Fnbytes[start + 2] = seg[1];
			Fnbytes[start + 3] = seg[2];
			Fnbytes[start + 4] = seg[3];

			free( seg );
		}
	}

	int fnPointerDataSize = header->fnArrSize * 4;
	int startOfStrings = header->fnArrPtr + fnDataSize + sizeOfVarInitialisers + Fnbytes.size( ) + fnPointerDataSize;

	//Get size of strings
	int sizeofstringarray = 0;
	for( int i = 0; i < m_vecMissionStrns.size( ); i++ )
	{
		int numChars = m_vecMissionStrns[i].size( );
		sizeofstringarray += ( sizeof( wchar_t )*numChars );
		sizeofstringarray += 2; //Zero terminator size
	}

	//Get size of variable name strings
	int sizeofvarstrarray = 0;
	for( int i = 0; i < m_vecVarNames.size( ); i++ )
	{
		int numChars = m_vecVarNames[i].size( );
		sizeofvarstrarray += ( sizeof( wchar_t )*numChars );
		sizeofvarstrarray += 2; //Zero terminator size
	}
	int startOfFunctionStrings = startOfStrings + sizeofstringarray + sizeofvarstrarray;

	header->strnPtr = startOfStrings;

	//Array of fn calls
	std::vector< char > fnPointerBytes;

	int offset = startOfFnData - ( startOfStrings - fnPointerDataSize );
	for( int i = 0; i < m_vecFunctions.size( ); i++ )
	{
		//offset -= 1;
		char *seg = IntToBytes( offset + 4 );
		fnPointerBytes.push_back( 0xA9 );
		fnPointerBytes.push_back( seg[0] );
		fnPointerBytes.push_back( seg[1] );
		fnPointerBytes.push_back( 0x30 );
		free( seg );

		int fnSize = m_vecFunctions[i]->bytes.size( );
		offset -= 4;
		offset += fnSize;
	}

	//FuncData bytes: 16 bytes per "line"
	std::vector< char > fnDataBytes;
	offset = 0;
	int strOfs = 0;
	for( int i = 0; i < m_vecFunctions.size( ); i++ )
	{
		std::vector< char > dataBytes = m_vecFunctions[i]->GenerateFnDataBytes( startOfFunctionStrings + strOfs, startOfStrings - fnPointerDataSize, i, offset );
		fnDataBytes.insert( fnDataBytes.end(), dataBytes.begin(), dataBytes.end() );

		offset += m_vecFunctions[i]->bytes.size( );
		strOfs += (sizeof( wchar_t )*m_vecFunctions[i]->fnName.size( )) + 2;
	}

	//Start filling out our bytes by generating the header
	std::vector< char > bytes = header->GenerateBytes();

	//Push variable name bytes
	offset = 0;
	for( int i = 0; i < m_vecVarNames.size( ); i++ )
	{
		char *seg = IntToBytes( startOfStrings + sizeofstringarray + offset );
		bytes.push_back( seg[0] );
		bytes.push_back( seg[1] );
		bytes.push_back( seg[2] );
		bytes.push_back( seg[3] );
		free( seg );

		int numChars = m_vecVarNames[i].size( );
		offset += ( sizeof( wchar_t )*numChars );
		offset += 2; //Zero terminator size
	}

	//Push function declaration bytes
	for( int i = 0; i < fnDataBytes.size( ); i++ )
	{
		bytes.push_back( fnDataBytes[i] );
	}

	//Push variable initialiser bytes
	bytes.push_back( 0x30 );

	//Push function bytes
	for( int i = 0; i < Fnbytes.size( ); i++ )
	{
		bytes.push_back( Fnbytes[i] );
	}

	//Push function pointers
	for( int i = 0; i < fnPointerBytes.size( ); i++ )
	{
		bytes.push_back( fnPointerBytes[i] );
	}

	//Push strings
	for( int i = 0; i < m_vecMissionStrns.size( ); i++ ) 
	{
		PushWStringToVector( m_vecMissionStrns[i], &bytes );
	}

	//Push var Strings
	for( int i = 0; i < m_vecVarNames.size( ); i++ )
	{
		PushWStringToVector( m_vecVarNames[i], &bytes );
	}

	//Push fn Strings
	for( int i = 0; i < m_vecFunctions.size( ); i++ ) 
	{
		PushWStringToVector( m_vecFunctions[i]->fnName, &bytes );
	}

	////Variable names
	//int varArrSize = m_vecVarNames.size();
	//int varArrPtr = 0x50;
	//seg = IntToBytes( varArrSize );

	//for( int i = 0; i < 4; i++ )
	//{
	//	bytes.push_back( seg[i] );
	//}

	//seg = IntToBytes( varArrPtr );
	//for( int i = 0; i < 4; i++ )
	//{
	//	bytes.push_back( seg[i] );
	//}

	//Functions

	//Final write.
	for( int i = 0; i < bytes.size(); i++ )
	{
		newMission << bytes[i];
	}

	newMission.close();

	delete header;
}

MissionFunction::MissionFunction( std::wstring srcCode, CMissionScript *script )
{
	sourceCode = srcCode;
	myScript = script;
}

void MissionFunction::CompileLine(std::wstring fnStrn)
{
	std::wstring argsStrn;
	bool parsingArgStrn = false;
	int bracketDepth = 0;

	std::wstring preBracketString;

	//Kill whitespace in main string.
	fnStrn = KillWhitespace(fnStrn);

	//Begin parsing string.
	for (int i = 0; i < fnStrn.size(); i++)
	{
		//Control chars
		if (fnStrn[i] == L'(')
		{
			if (bracketDepth == 0)
			{
				preBracketString = fnStrn.c_str();
				preBracketString.erase(i, preBracketString.size() - i);
				i++;
			}

			parsingArgStrn = true;
			bracketDepth++;
		}
		if (fnStrn[i] == L')')
		{
			bracketDepth--;
			if (bracketDepth <= 0)
			{
				parsingArgStrn = false;
				i++;
			}
		}

		if (parsingArgStrn)
		{
			argsStrn += fnStrn[i];
		}
	}

	//No brackets?
	if (preBracketString.size() == 0)
		preBracketString = fnStrn;

	//Scan string for variable operators
	bool lineSetsVariable = false;
	bool lineSetsLocalVariable = false;
	std::wstring strVarName;
	for (int pos = 0; pos < preBracketString.size(); pos++)
	{
		if (preBracketString[pos] == L'=')
		{
			//Get left side:
			strVarName = preBracketString.c_str();
			strVarName.erase(pos, strVarName.size() - pos);
			preBracketString.erase(0, pos + 1);

			lineSetsVariable = true;
			break;
		}
	}

	//Todo: Handle variables correctly.
	if (lineSetsVariable)
	{
		//Lets handle this as local variable for now.
		bool isDefinedLocalVar = false;
		for (int i = 0; i < m_vecLocalVars.size(); i++)
		{
			if (m_vecLocalVars[i] == strVarName)
			{
				bytes.push_back(0x58);
				bytes.push_back(i + 1);

				isDefinedLocalVar = true;
				lineSetsLocalVariable = true;
			}
		}
		if (!isDefinedLocalVar)
		{
			bool isDefinedGlobalVar = false;

			for (int i = 0; i < myScript->GetNumVars(); i++)
			{
				if (myScript->GetVarName(i) == strVarName)
				{
					bytes.push_back(0x55);
					bytes.push_back(i);

					isDefinedGlobalVar = true;
					break;
				}
			}
			if (!isDefinedGlobalVar)
			{
				m_vecLocalVars.push_back(strVarName);
				m_iNumLocalVars++;

				bytes.push_back(0x58);
				bytes.push_back(m_iNumLocalVars);

				lineSetsLocalVariable = true;
			}
		}

		//Global variable (RAS Absolute)
		//bytes.push_back( 0x16 );
		//bytes.push_back( 0x00 );
	}

	//Check functions
	for (int j = 0; j < myScript->GetNumFunctions(); j++)
	{
		if (preBracketString == myScript->GetFnName(j)) //If I am trying to call a function I have stored locally:
		{
			//Create string backup
			std::wstring strnBak = preBracketString.c_str();

			//Handle the rest of the function
			CompileArgs(argsStrn);

			//Register area for patching
			patchBytePos.push_back(bytes.size());
			patchFuncName.push_back(strnBak);

			//Prepare dummy jump
			//Assume largest type for now.
			bytes.push_back(0xE9);

			bytes.push_back(0x00);
			bytes.push_back(0x00);
			bytes.push_back(0x00);
			bytes.push_back(0x00);
		}
	}

	//Get function name:
	if (preBracketString == L"2C" || preBracketString == L"2c" || preBracketString == L"syscall0") //2C
	{
		//Get arguement 1 (Type of 2C command)
		std::wstring arg = SimpleTokenise(argsStrn, L',');

		int parseNum = (int)wcstol(arg.c_str(), NULL, 0);
		char* fnTypeBytes = IntToBytes(parseNum);

		//Handle the rest of the function
		CompileArgs(argsStrn);

		//Compress bytes
		if (parseNum > 0xff)
		{
			bytes.push_back(0xAC);
			bytes.push_back(fnTypeBytes[0]);
			bytes.push_back(fnTypeBytes[1]);
		}
		else
		{
			bytes.push_back(0x6C);
			bytes.push_back(fnTypeBytes[0]);
		}
		free(fnTypeBytes);
	}
	else if (preBracketString == L"2D" || preBracketString == L"2d" || preBracketString == L"syscall1") //2D
	{
		//Get arguement 1 (Type of 2D command)
		std::wstring arg = SimpleTokenise(argsStrn, L',');

		int parseNum = (int)wcstol(arg.c_str(), NULL, 0);
		char* fnTypeBytes = IntToBytes(parseNum);

		//Handle the rest of the function
		CompileArgs(argsStrn);

		//Compress bytes
		if (parseNum > 0xff)
		{
			bytes.push_back(0xAD);
			bytes.push_back(fnTypeBytes[0]);
			bytes.push_back(fnTypeBytes[1]);
		}
		else
		{
			bytes.push_back(0x6D);
			bytes.push_back(fnTypeBytes[0]);
		}
		free(fnTypeBytes);
	}
	else if (preBracketString == L"2E" || preBracketString == L"2e" || preBracketString == L"syscall2") //2E
	{
		//Get arguement 1 (Type of 2E command)
		std::wstring arg = SimpleTokenise(argsStrn, L',');

		int parseNum = (int)wcstol(arg.c_str(), NULL, 0);
		char* fnTypeBytes = IntToBytes(parseNum);

		//Handle the rest of the function
		CompileArgs(argsStrn);

		//Compress bytes
		if (parseNum > 0xff)
		{
			bytes.push_back(0xAE);
			bytes.push_back(fnTypeBytes[0]);
			bytes.push_back(fnTypeBytes[1]);
		}
		else
		{
			bytes.push_back(0x6E);
			bytes.push_back(fnTypeBytes[0]);
		}
		free(fnTypeBytes);
	}
	else if (preBracketString == L"syscallF" || preBracketString == L"syscallf" || preBracketString == L"syscall3") //2F
	{
		//Get arguement 1 (Type of 2F command)
		std::wstring arg = SimpleTokenise(argsStrn, L',');

		int parseNum = (int)wcstol(arg.c_str(), NULL, 0);
		char* fnTypeBytes = IntToBytes(parseNum);

		//Handle the rest of the function
		CompileArgs(argsStrn);

		//Compress bytes
		if (parseNum > 0xff)
		{
			bytes.push_back(0xAF);
			bytes.push_back(fnTypeBytes[0]);
			bytes.push_back(fnTypeBytes[1]);
		}
		else
		{
			bytes.push_back(0x6F);
			bytes.push_back(fnTypeBytes[0]);
		}
		free(fnTypeBytes);
	}
	else if (preBracketString == L"usetemplist") //temp list
	{
		// use a temp list to solve hex() does not have variables
		CompileArgs(argsStrn);
	}
	else if (preBracketString == L"return") //Return
	{
		if (bytes.size() < 127)
		{
			bytes.push_back(0x68);
			bytes.push_back(((-bytes.size()) + 1));
		}
		else
		{
			bytes.push_back(0xA8);

			char* ofsBytes = IntToBytes((-bytes.size()) + 1);

			bytes.push_back(ofsBytes[0]);
			bytes.push_back(ofsBytes[1]);

			free(ofsBytes);
		}
	}
	else if (preBracketString == L"if") //conditional
	{
		ifArgs.push_back(argsStrn.c_str());

		CompileExpression(argsStrn);

		bytes.push_back(0xE6); //Jump if false
		//Placeholder
		bytes.push_back(0x00);
		bytes.push_back(0x00);
		bytes.push_back(0x00);
		bytes.push_back(0x00);
		patchConditionalPos.push_back(bytes.size());
	}
	else if (preBracketString == L"}") //Patch conditional
	{
		if (whileStartPos.size() > 0)
		{
			bytes.push_back(0xA8);

			int ofs = whileStartPos.back() - bytes.size();
			char* ofsBytes = IntToBytes(ofs);

			bytes.push_back(ofsBytes[0]);
			bytes.push_back(ofsBytes[1]);

			free(ofsBytes);

			whileStartPos.pop_back();
		}

		if (patchConditionalPos.size() > 0)
		{
			int start = patchConditionalPos.back();

			if (elseConditionalPos.size() > 0 && int(bytes[start - 5]) == -24 && int(bytes[start - 2]) == 0 && int(bytes[start - 1]) == 0)
			{
				char* seg = IntToBytes((bytes.size() - start) + 5);
				//If true, skip the content of else
				bytes[start - 4] = seg[0];
				bytes[start - 3] = seg[1];
				bytes[start - 2] = seg[2];
				bytes[start - 1] = seg[3];

				free(seg);

				elseConditionalPos.pop_back();
			}
			else
			{
				bytes.push_back(0xE8);
				bytes.push_back(0x05);
				bytes.push_back(0x00);
				bytes.push_back(0x00);
				bytes.push_back(0x00);

				if (int(bytes[start - 5]) == -26 && int(bytes[start - 2]) == 0 && int(bytes[start - 1]) == 0)
				{
					char* seg = IntToBytes((bytes.size() - start) + 5);

					bytes[start - 4] = seg[0];
					bytes[start - 3] = seg[1];
					bytes[start - 2] = seg[2];
					bytes[start - 1] = seg[3];

					free(seg);
				}
				else
				{
					std::wcout << int(bytes[start - 5]);
					std::wcout << int(bytes[start - 4]);
					std::wcout << int(bytes[start - 3]);
					std::wcout << int(bytes[start - 2]);
					std::wcout << int(bytes[start - 1]);
					//system("pause");
					//exit(0);
				}
			}

			patchConditionalPos.pop_back();
		}

		// There is a problem
		/*
		if (ifConditionalPos.size() > 0)
		{
			bytes.push_back(0xE8);
			bytes.push_back(0x05);
			bytes.push_back(0x00);
			bytes.push_back(0x00);
			bytes.push_back(0x00);

			int start = ifConditionalPos.back();

			int truepos;
			if (ifConditionalPos.size() < patchConditionalPos.size())
			{
				truepos = patchConditionalPos.back();
			}
			else
			{
				truepos = start;
			}

			char* seg = IntToBytes((bytes.size() - truepos) + 1);

			bytes[start + 0] = seg[0];
			bytes[start + 1] = seg[1];
			bytes[start + 2] = seg[2];
			bytes[start + 3] = seg[3];

			free(seg);

			patchConditionalPos.pop_back();
			ifConditionalPos.pop_back();
		}

		if (elseConditionalPos.size() > 0)
		{
			int start = elseConditionalPos.back();

			if (bytes[start - 5] == 0xE8 && bytes[start - 2] == 0x00 && bytes[start - 1] == 0x00)
			{
				int truepos;
				if (elseConditionalPos.size() < patchConditionalPos.size())
				{
					truepos = patchConditionalPos.back();
				}
				else
				{
					truepos = start;
				}

				char* seg = IntToBytes((bytes.size() - truepos) + 5);
				//If true, skip the content of else
				bytes[start - 4] = seg[0];
				bytes[start - 3] = seg[1];
				bytes[start - 2] = seg[2];
				bytes[start - 1] = seg[3];

				free(seg);

				patchConditionalPos.pop_back();
				elseConditionalPos.pop_back();
			}
		}*/
	}
	else if( preBracketString == L"while" ) //while loop
	{
		whileStartPos.push_back( bytes.size( ) );
		CompileExpression( argsStrn );

		bytes.push_back( 0xe6 ); //Jump if stack == 0

		patchConditionalPos.push_back( bytes.size( ) );

		//Placeholder
		bytes.push_back( 0x00 );
		bytes.push_back( 0x00 );
		bytes.push_back( 0x00 );
		bytes.push_back( 0x00 );
	}
	else if (preBracketString == L"else") //else
	{
		//elseConditionalPos is necessary
		//it needs to be used to judge "else" instead of "if"
		elseConditionalPos.push_back(bytes.size());
		patchConditionalPos.push_back(bytes.size());

		CompileExpression(argsStrn);
	}
	else if( preBracketString == L"hex" ) //Raw hex code
	{
		//Align to 2-byte chunks
		if( argsStrn.length( ) % 2 > 0 )
		{
			argsStrn = (L"0" + argsStrn);
		}

		//Convert to hex.
		for( unsigned int i = 0; i < argsStrn.length( ); i += 2 )
		{
			std::wstring byteString = argsStrn.substr( i, 2 );
			char byte = (char)std::wcstol( byteString.c_str( ), NULL, 16 );
			bytes.push_back( byte );
		}
	}
	else //Expression
	{
		if( lineSetsVariable )
		{
			//look for control symbols:
			CompileExpression( preBracketString );
		}
	}

	if( lineSetsVariable )
	{
		bytes.push_back( 0x36 );
	}
}

void MissionFunction::CompileExpression( std::wstring argsStrn )
{
	//Parse string, scaning for specific expressions
	std::wstring token, expr;
	expr = argsStrn.c_str( );

	bool handledExpr = false;

	//delete ;
	if( expr[expr.size( )] == L';' )
	{
		expr.pop_back( );
	}

	//If I am a string, just ignore it.
	if( !( DetermineType( argsStrn ) == T_STRING ) )
	{
		// OK! Here is the main problem child.
		// We need to handle conditionals elsewhere, perhaps? That said, they do "operate" two values together into a single value. But it is hard to really handle 1 + 2 > 5 * 3, as it will become 0 * 3...
		// CompileArgs can convert any value into its bvm equivalent, so we should exploit that here.
		// 
		// 
		// BVM bytes required:
		// 0x4 (0x44): BVM add, extra param determines type ( 0 = int + int, 1 = int + float, 2 = float + int, 3 = float + float )
		// 0x5 (0x45): BVM sub, extra param determines type ( 0 = int - int, 1 = int - float, 2 = float - int, 3 = float - float )
		// 0x6 (0x46): BVM mul, extra param determines type ( 0 = int * int, 1 = int * float, 2 = float * int, 3 = float * float )
		// 0x7 (0x47): BVM div, extra param determines type ( 0 = int / int, 1 = int / float, 2 = float / int, 3 = float / float )
		// 
		// Solution 1:
		// Tokenise, extracting the two halfs of the current step
		// Feed data into stack, 'next' first, then 'part'.
		// Determine value types of both "next" and "Part"
		// Take control value and fill in the blanks.
		// Problems: "Wrong way around"
		// Will likely need to do this in reverse.
		//
		//
		//
		// Solution 2:
		// iterate from string len to 0:
		// if char at index = control symbol.
		//
		// if we dont have a control symbol yet:
		// chop off end of string, store it as 'part', store control symbol.
		// Determine value type of part. Store result.
		// Feed part to CompileArgs
		//
		// Otherwise:
		// chop off end of string, store it as 'next', store control symbol to temp value.
		// Determine value type of next. Store result.
		// Feed next to CompileArgs
		// Generate arithematic control bytes and feed to BVM.
		// Swap part with next. Swap stored control symbol with next.

		// When we reach 0, do pretty much the same thing, just with the remainder of the string as the next.
		// Should have handled it then.

		//IMPLEMENTATION:

		//Local storage
		wchar_t ctrl = NULL;
		wchar_t delim[4] = { L'+', L'-', L'*', L'/' };
		int lastnumber = 0;

		std::wstring next, part, last;
		
		ValueType lastType;

		//Must get the length of the first value read forward.
		part = expr;
		for (int i = part.size() - 1; i > 0; --i)
		{
			for (int delim_index = 0; delim_index < 4; ++delim_index)
			{
				if (part[i] == delim[delim_index]) //if char at index = control symbol.
				{
					part.erase(i, part.size() - i);
				}
			}
		}

		//Read the string forward, prevent reverse output.
		//i should be 1, as 0 may cause negative numbers to be recognized as -
		for (int i = 1; i < expr.size(); ++i)
		{
			for (int delim_index = 0; delim_index < 4; ++delim_index)
			{
				if (expr[i] == delim[delim_index]) //if char at index = control symbol.
				{
					if (ctrl == NULL) //if we dont have a control symbol yet:
					{
						//Store control symbol. Chop off end of string, store it as 'part'.
						ctrl = expr[i];

						//Determine value type of part. Store result.
						//Is actually the first value
						lastType = DetermineType(part);

						//TODO: Determine variable types appropriately.
						if (lastType == T_INVALID)
							lastType = T_INT;

						//Feed part to CompileArgs
						CompileValue(part);

						//For testing output order
						//bytes.push_back(0xF1);

						//Because the previous pointer is needed, it is stored.
						lastnumber = i;
					}
					else //Otherwise:
					{
						//Store control symbol to temp buffer.
						char newctrl = expr[i];

						//Chop off the tail and head, keep the required string, store it as 'next'.
						//Current pointer is next position, so use previous pointer.
						next = expr.c_str();
						next.erase(i, expr.size() - i);
						next.erase(0, lastnumber + 1);

						//Determine value type of next. Store result.
						ValueType curType = DetermineType(next);

						//TODO: Determine variable types appropriately.
						if (curType == T_INVALID)
							curType = T_INT;

						//Feed next to CompileArgs
						CompileValue(next);

						//Generate arithematic control bytes and feed to BVM.

						char subByte;

						//Eh, probably a better way of doing this.
						if (lastType == T_INT && curType == T_INT)
							subByte = 0; //Int + Int
						else if (lastType == T_INT && curType == T_FLOAT)
							subByte = 1; //Int + Float
						else if (lastType == T_FLOAT && curType == T_INT)
							subByte = 2; //Float + Int
						else if (lastType == T_FLOAT && curType == T_FLOAT)
							subByte = 3; //Float + Float

						//Can safely assume > 0 is a float.
						if (subByte > 0)
							lastType = T_FLOAT;

						char mainByte;

						//Convert char to appropriate byte
						switch (ctrl)
						{
						case '+':
							mainByte = 0x04;
							break;
						case '-':
							mainByte = 0x05;
							break;
						case '*':
							mainByte = 0x06;
							break;
						case '/':
							mainByte = 0x07;
							break;
						}

						//Push to compiled code
						bytes.push_back(mainByte);
						bytes.push_back(subByte);

						//Store control symbol.
						ctrl = newctrl;
						//Because the previous pointer is needed, it is stored.
						lastnumber = i;
					}
				}
			}
		}

		//iterate from string len to 0. Not using it now.
		/*
			for (int i = expr.size() - 1; i > 0; --i)
			{
				for (int delim_index = 0; delim_index < 4; ++delim_index)
				{
					if (expr[i] == delim[delim_index]) //if char at index = control symbol.
					{
						if (ctrl == NULL) //if we dont have a control symbol yet:
						{
							//Store control symbol. Chop off end of string, store it as 'part'.
							ctrl = expr[i];

							part = expr.c_str();
							expr.erase(i, expr.size() - i);
							part.erase(0, i + 1);

							//Determine value type of part. Store result.
							lastType = DetermineType(part);

							//TODO: Determine variable types appropriately.
							if (lastType == T_INVALID)
								lastType = T_INT;

							//Feed part to CompileArgs
							CompileValue(part);
						}
						else //Otherwise:
						{
							//Store control symbol to temp buffer.
							char newctrl = expr[i];

							//Chop off end of string, store it as 'next'.
							next = expr.c_str();
							expr.erase(i, expr.size() - i);
							next.erase(0, i + 1);

							//Determine value type of next. Store result.
							ValueType curType = DetermineType(next);

							//TODO: Determine variable types appropriately.
							if (curType == T_INVALID)
								curType = T_INT;

							//Feed next to CompileArgs
							CompileValue(next);

							//Generate arithematic control bytes and feed to BVM.

							char subByte;

							//Eh, probably a better way of doing this.
							if (lastType == T_INT && curType == T_INT)
								subByte = 0; //Int + Int
							else if (lastType == T_INT && curType == T_FLOAT)
								subByte = 1; //Int + Float
							else if (lastType == T_FLOAT && curType == T_INT)
								subByte = 2; //Float + Int
							else if (lastType == T_FLOAT && curType == T_FLOAT)
								subByte = 3; //Float + Float

							//Can safely assume > 0 is a float.
							if (subByte > 0)
								lastType = T_FLOAT;

							char mainByte;

							//Convert char to appropriate byte
							switch (ctrl)
							{
							case '+':
								mainByte = 0x04;
								break;
							case '-':
								mainByte = 0x05;
								break;
							case '*':
								mainByte = 0x06;
								break;
							case '/':
								mainByte = 0x07;
								break;
							}

							//Push to compiled code
							bytes.push_back(mainByte);
							bytes.push_back(subByte);

							//Store control symbol.
							ctrl = newctrl;
						}
					}
				}
			}
		*/

		//When we reach 0, do pretty much the same thing, just with the remainder of the string as the next.
		if( ctrl != NULL )
		{
			//Chop off unwanted strings
			last = expr.c_str();
			last.erase(0, lastnumber + 1);
			//Determine value type of next. Store result.
			ValueType curType = DetermineType(last);

			//TODO: Determine variable types appropriately.
			if( curType == T_INVALID )
				curType = T_INT;

			//Feed next to CompileArgs
			CompileValue(last);

			//Generate arithematic control bytes and feed to BVM.

			char subByte;

			//Eh, probably a better way of doing this.
			if( lastType == T_INT && curType == T_INT )
				subByte = 0; //Int + Int
			else if( lastType == T_INT && curType == T_FLOAT )
				subByte = 1; //Int + Float
			else if( lastType == T_FLOAT && curType == T_INT)
				subByte = 2; //Float + Int
			else if( lastType == T_FLOAT && curType == T_FLOAT)
				subByte = 3; //Float + Float

			char mainByte;
			//Used to distinguish who is the last output.
			switch(ctrl)
			{
			case '+':
			mainByte = 0x44;
			break;
			case '-':
			mainByte = 0x45;
			break;
			case '*':
			mainByte = 0x46;
			break;
			case '/':
			mainByte = 0x47;
			break;
			}

			bytes.push_back( mainByte );
			bytes.push_back( subByte );

			handledExpr = true;
		}

		//Greater than
		token = SimpleTokenise( expr, L'>' );
		if( expr.size( ) > 0 )
		{
			if( expr.front( ) == L'=' )
			{
				expr.erase( 0, 1 );

				if( token.find( L"2C" ) != std::string::npos || token.find( L"2D" ) != std::string::npos )
					CompileLine( token );
				else
					CompileArgs( token );

				if( expr.find( L"2C" ) != std::string::npos || expr.find( L"2D" ) != std::string::npos )
					CompileLine( expr );
				else
					CompileArgs( expr );

				bytes.push_back( 0x24 );
				bytes.push_back( 0x00 );
				handledExpr = true;
			}
			else
			{
				if( token.find( L"2C" ) != std::string::npos || token.find( L"2D" ) != std::string::npos )
					CompileLine( token );
				else
					CompileArgs( token );

				if( expr.find( L"2C" ) != std::string::npos || expr.find( L"2D" ) != std::string::npos )
					CompileLine( expr );
				else
					CompileArgs( expr );

				bytes.push_back( 0x25 );
				bytes.push_back( 0x00 );
				handledExpr = true;
			}
		}

		expr = argsStrn.c_str( );
		//Less than
		token = SimpleTokenise( expr, L'<' );
		if( expr.size( ) > 0 )
		{
			if( expr.front( ) == L'=' )
			{
				expr.erase( 0, 1 );

				if( token.find( L"2C" ) != std::string::npos || token.find( L"2D" ) != std::string::npos )
					CompileLine( token );
				else
					CompileArgs( token );

				if( expr.find( L"2C" ) != std::string::npos || expr.find( L"2D" ) != std::string::npos )
					CompileLine( expr );
				else
					CompileArgs( expr );

				bytes.push_back( 0x21 );
				bytes.push_back( 0x00 );
				handledExpr = true;
			}
			else
			{
				if( token.find( L"2C" ) != std::string::npos || token.find( L"2D" ) != std::string::npos )
					CompileLine( token );
				else
					CompileArgs( token );

				if( expr.find( L"2C" ) != std::string::npos || expr.find( L"2D" ) != std::string::npos )
					CompileLine( expr );
				else
					CompileArgs( expr );

				bytes.push_back( 0x20 );
				bytes.push_back( 0x00 );
				handledExpr = true;
			}
		}

		expr = argsStrn.c_str( );
		//Equal to:
		token = SimpleTokenise( expr, L'=' );
		if( expr.size( ) > 0 )
		{
			if( expr.front( ) == L'=' )
			{
				expr.erase( 0, 1 );

				if( token.find( L"2C" ) != std::string::npos || token.find( L"2D" ) != std::string::npos )
					CompileLine( token );
				else
					CompileArgs( token );

				if( expr.find( L"2C" ) != std::string::npos || expr.find( L"2D" ) != std::string::npos )
					CompileLine( expr );
				else
					CompileArgs( expr );

				bytes.push_back( 0x22 );
				bytes.push_back( 0x00 );
				handledExpr = true;
			}
		}

		expr = argsStrn.c_str( );
		//Not
		token = SimpleTokenise( expr, L'!' );
		if( expr.size( ) > 0 )
		{
			if( token.find( L"2C" ) != std::string::npos || token.find( L"2D" ) != std::string::npos )
				CompileLine( token );
			else
				CompileArgs( token );

			if( expr.front( ) == L'=' ) // Not equal to:
			{
				expr.erase( 0, 1 );

				if( expr.find( L"2C" ) != std::string::npos || expr.find( L"2D" ) != std::string::npos )
					CompileLine( expr );
				else
					CompileArgs( expr );

				bytes.push_back( 0x23 );
				bytes.push_back( 0x00 );
				handledExpr = true;
			}
			else //Raw Not:
			{
				handledExpr = true;
				bytes.push_back( 0x11 );
			}
		}

		expr = argsStrn.c_str( );
		//And
		token = SimpleTokenise( expr, L'&' );
		if( expr.size( ) > 0 )
		{
			if( token.find( L"2C" ) != std::string::npos || token.find( L"2D" ) != std::string::npos )
				CompileLine( token );
			else
				CompileArgs( token );

			if( expr.back( ) == L'&' ) //Logical and
			{
				expr.erase( 0, 1 );

				if( expr.find( L"2C" ) != std::string::npos || expr.find( L"2D" ) != std::string::npos )
					CompileLine( expr );
				else
					CompileArgs( expr );

				bytes.push_back( 0x0E );
				handledExpr = true;
			}
			else //Raw And:
			{
				handledExpr = true;
				bytes.push_back( 0x0E );
			}
		}

		expr = argsStrn.c_str();
	}

	//OLD CODE: DISABLED.
#if 0
	//+
	//++, search for ++thing or thing++
	if( ( expr[0] == L'+' && expr[1] == L'+' ) || expr[expr.size( )] == L'+' && expr[expr.size( ) - 1] == L'+' )
	{


		//0x0A
	}
	else
	{
		expr = argsStrn.c_str( );
		token = SimpleTokenise( expr, L'+' );
		if( expr.size( ) > 0 )
		{
			if( IsValidInt( token ) && IsValidInt( expr ) )
			{
				CompileArgs( ToString( stoi( token ) + stoi( expr ) ) );
			}
			else
			{
				bool handledVar;
				//Then check if its a global var
				for( int i = 0; i < myScript->GetNumVars( ); i++ )
				{
					if( myScript->GetVarName( i ) == token )
					{
						//bytes.push_back( 0x55 );
						//bytes.push_back( i );
						bytes.push_back( 0x54 );
						bytes.push_back( i );

						handledVar = true;
					}
				}

				if( handledVar )
				{
					bytes.push_back( 0x55 );
					bytes.push_back( stoi( expr ) );
					bytes.push_back( 0x4 );
					bytes.push_back( 0x0 );
				}
			}
		}
	}

	//-
	//--, search for ++thing or thing++
	if( ( expr[0] == L'-' && expr[1] == L'-' ) || expr[expr.size( )] == L'-' && expr[expr.size( ) - 1] == L'-' )
	{


		//0x0A
	}
	else
	{
		expr = argsStrn.c_str( );
		token = SimpleTokenise( expr, L'-' );
		if( expr.size( ) > 0 )
		{
			if( IsValidInt( token ) && IsValidInt( expr ) )
			{
				CompileArgs( ToString( stoi( token ) + stoi( expr ) ) );
			}
			else
			{
				bool handledVar;
				//Then check if its a global var
				for( int i = 0; i < myScript->GetNumVars( ); i++ )
				{
					if( myScript->GetVarName( i ) == token )
					{
						//bytes.push_back( 0x55 );
						//bytes.push_back( i );
						bytes.push_back( 0x54 );
						bytes.push_back( i );

						handledVar = true;
					}
				}

				if( handledVar )
				{
					bytes.push_back( 0x55 );
					bytes.push_back( stoi( expr ) );
					bytes.push_back( 0x5 );
					bytes.push_back( 0x0 );
				}
			}
		}
	}
#endif

	if( !handledExpr )
	{
		if( argsStrn.size( ) > 0 )
		{
			ValueType type = DetermineType( argsStrn );
			if( type != T_INVALID )
			{
				CompileValue( argsStrn );
			}
			else
			{
				//Check local and global variables for a match:

				bool handledVar = false;
				for( int i = 0; i < m_vecLocalVars.size( ); i++ )
				{
					if( argsStrn == m_vecLocalVars[i] )
					{
						handledVar = true;
					}
				}

				//Then check if its a global var
				for( int i = 0; i < myScript->GetNumVars( ); i++ )
				{
					if( myScript->GetVarName( i ) == argsStrn )
					{
						handledVar = true;
					}
				}

				if( handledVar )
				{
					CompileValue( argsStrn );
				}
				else
				{
					CompileLine(argsStrn);
				}
			}
		}
	}
}

void MissionFunction::CompileArgs( std::wstring argsStrn )
{
	while( argsStrn.size( ) > 0 )
	{
		std::wstring arg;
		arg = SimpleTokenise( argsStrn, L',' );

		CompileExpression( arg );
	}
}

//Compiles a single value to its BVM equivalent
void MissionFunction::CompileValue( std::wstring argsStrn )
{
	//Proccess arguement:
	//First, see if its a local variable:
	bool handledVar = false;
	for( int j = 0; j < m_vecLocalVars.size( ); j++ )
	{
		if( argsStrn == m_vecLocalVars[j] )
		{
			bytes.push_back( 0x57 );
			bytes.push_back( j + 1 );
			handledVar = true;
		}
	}

	//Then check if its a global var
	for( int i = 0; i < myScript->GetNumVars( ); i++ )
	{
		if( myScript->GetVarName( i ) == argsStrn )
		{
			//bytes.push_back( 0x55 );
			//bytes.push_back( i );
			bytes.push_back( 0x54 );
			bytes.push_back( i );

			handledVar = true;
		}
	}

	if( handledVar )
	{
		return;
	}

	if( ( argsStrn.front( ) == L'\"' && argsStrn.back( ) == L'\"' ) || ( argsStrn.front( ) == L'L' && argsStrn[1] == L'\"' && argsStrn.back( ) == L'\"' ) ) //String
	{
		//Get rid of ""
		argsStrn.pop_back( );
		argsStrn.erase( argsStrn.begin( ) );

		//Check string array:
		bool found = false;
		int ofs = 0;
		for( int strID = 0; strID < myScript->m_vecMissionStrns.size( ); strID++ )
		{
			if( myScript->m_vecMissionStrns[strID] == argsStrn )
			{
				found = true;
				break;
			}
			ofs += sizeof( wchar_t )*myScript->m_vecMissionStrns[strID].size( );
			ofs += 2; //0 terminator size
		}
		if( !found )
		{
			myScript->m_vecMissionStrns.push_back( argsStrn );
		}

		//Run a few tests and optimise bytes.
		if( ofs == 0 )
		{
			bytes.push_back( 0x1a );
		}
		else
		{
			char *ofsBytes = IntToBytes( ofs );

			if( ofsBytes[1] == 0x00 && ofsBytes[2] == 0x00 && ofsBytes[3] == 0x00 ) //Probably a better way of checking this lol
			{
				bytes.push_back( 0x5a );
				bytes.push_back( ofsBytes[0] );
			}
			else
			{
				bytes.push_back( 0x9a );
				bytes.push_back( ofsBytes[0] );
				bytes.push_back( ofsBytes[1] );
			}

			free( ofsBytes );
		}
	}
	else if( argsStrn.back( ) == L'f' ) //Float
	{
		float num = stof( argsStrn );
		bytes.push_back( 0xD5 );
		char floatBytes[4] = { 0x0,0x0,0x0,0x0 };
		memcpy( floatBytes, &num, sizeof( num ) );

		bytes.push_back( floatBytes[0] );
		bytes.push_back( floatBytes[1] );
		bytes.push_back( floatBytes[2] );
		bytes.push_back( floatBytes[3] );
	}
	else //user *probably* wants an int
	{
		int num = stoi( argsStrn );
		if( num > 127 || num < -127 )
		{
			char *ofsBytes = IntToBytes( num );
			bytes.push_back( 0x95 );
			bytes.push_back( ofsBytes[0] );
			bytes.push_back( ofsBytes[1] );
			free( ofsBytes );
		}
		else
		{
			bytes.push_back( 0x55 );
			bytes.push_back( num );
		}
	}
}

//I dont like the name.
std::wstring TokeniseStringIgnoreQuotes( std::wstring &input, std::wstring tokens )
{
	bool bIsInQuote = false;
	wchar_t lastChar = 0;

	//Iterate over every character in the string.
	for( int pos = 0; pos < input.size( ); ++pos )
	{
		//Determine if this character is valid for splitting:
		if( !bIsInQuote )
		{
			//Iterate through every token character
			for( int i = 0; i < tokens.size(); ++i )
			{
				if( input[pos] == tokens[i] )
				{
					//Get left side:
					std::wstring out = input.c_str( );
					out.erase( pos, out.size( ) - pos );
					input.erase( 0, pos + 1 );

					//if( out.size() > 0 || input.size() == 0 )
					return out;
				}
			}
		}

		if( input[pos] == L'\"' && lastChar != '\\' )
		{
			bIsInQuote = !bIsInQuote;
		}

		lastChar = input[pos];
	}

	std::wstring out = input.c_str( );
	input.clear( );
	return out;
}

void MissionFunction::Compile( )
{
	std::wstring source = sourceCode;

	//Tokenise:
	std::wstring token;
	token = TokeniseStringIgnoreQuotes( source, L"\n" );

	std::vector< std::wstring > lines;
	//Collect all tokens
	while( source.size() > 0 || token.size() > 0 )
	{
		if( token.size() > 0 )
		lines.push_back( token );

		//FIXME: Problematic. Will probably need to create a custom tokenisation proccess here.
		token = TokeniseStringIgnoreQuotes( source, L"\n;" );
	}

	//Failsafe of sorts, incase trash bytes somehow make it this far.
	if( lines[0][0] == 0xfeff )
	{
		lines.erase( lines.begin() );
	}

	//Further split string:
	wchar_t* token2;
	std::wstring parser;
	std::wstring lineZeroBackup = &lines[0][0];
	token2 = wcstok( &lines[0][0], L"()," );

	//Assume First string is function name for now.
	//Scan string to split it into function name and type
	int fnRetType = 0; //0 = Void, only void for now
	token2 = wcstok( token2, L" " );
	parser = token2;

	//Check return types:
	if( parser == L"void" )
		fnRetType = 0;

	token2 = wcstok( NULL, L"(), " );

	//We will allow a type assumption of void.
	if( token2 == NULL )
		fnName = parser;
	else
		fnName = token2;

	//Verbose
	std::wcout << L"Compiling function: " + fnName + L"\n";

	m_iNumLocalVars = 0;

	//Get fn args.
	token2 = wcstok( &lineZeroBackup[0], L"(), " );
	while( token2 )
	{
		std::wstring argParser = token2;
		if( argParser == L"float" )
		{
			token2 = wcstok( NULL, L"(), " );
			if( token2 )
			{
				//Has float arguement
				m_iNumLocalVars++;
				m_vecLocalVars.push_back( token2 );
			}
		}
		else if( argParser == L"int" )
		{
			token2 = wcstok( NULL, L"(), " );
			if( token2 )
			{
				//Has float arguement
				m_iNumLocalVars++;
				m_vecLocalVars.push_back( token2 );
			}
		}
		else if( argParser == L"string" )
		{
			token2 = wcstok( NULL, L"(), " );
			if( token2 )
			{
				//Has string arguement (Todo, actualy parse this correctly?)
				m_iNumLocalVars++;
				m_vecLocalVars.push_back( token2 );
			}
		}
		token2 = wcstok( NULL, L"(), " );
	}


	//Prepare function return bytes
	//Return function
	bytes.push_back( 0x17 );
	bytes.push_back( 0x5B );
	bytes.push_back( 0x01 );
	bytes.push_back( 0x2A );

	//Prepare arguement parsing bytes
	//Todo: Make this generate using actual data
	bytes.push_back( 0x5C );
	bytes.push_back( 0x01 );
	bytes.push_back( 0x19 );

	//FnArgs
	for( int i = m_iNumLocalVars; i > 0; i-- )
	{
		bytes.push_back( 0x59 );
		bytes.push_back( i );
	}

	//Parse every line
	for( int i = 0; i < lines.size( ); i++ )
	{
		//Attempt rewrite code
		CompileLine( lines[i] );
	}

	//Conditional jump to return.

	if( bytes.size( ) < 127 )
	{
		bytes.push_back( 0x68 );
		bytes.push_back( ( ( -bytes.size( ) ) + 1 ) );
	}
	else
	{
		bytes.push_back( 0xA8 );

		char *ofsBytes = IntToBytes( ( -bytes.size( ) ) + 1 );

		bytes.push_back( ofsBytes[0] );
		bytes.push_back( ofsBytes[1] );

		free( ofsBytes );
	}

	bytes[2] = m_iNumLocalVars + 1;
	bytes[5] = m_iNumLocalVars + 1;
}

void MissionFunction::Decompile( )
{
}

//Todo: Clean this up a bit
void MissionFunction::GetName( )
{
	std::wstring source = sourceCode.c_str( );

	//Tokenise:
	wchar_t* token;
	token = wcstok( &source[0], L"\n" );

	std::vector< std::wstring > lines;
	//Collect all tokens
	while( token )
	{
		lines.push_back( token );
		token = wcstok( NULL, L"\n;" );
	}

	//Further split string:
	wchar_t* token2;
	std::wstring parser;
	token2 = wcstok( &lines[0][0], L"()," );

	//Assume First string is function name for now.
	//Scan string to split it into function name and type
	int fnRetType = 0; //0 = Void, only void for now
	token2 = wcstok( token2, L" " );
	parser = token2;

	//Check return types:
	if( parser == L"void" )
		fnRetType = 0;

	token2 = wcstok( NULL, L" " );

	//We will allow a type assumption of void.
	if( token2 == NULL )
		fnName = parser;
	else
		fnName = token2;
}

std::vector< char > MissionFunction::GenerateFnDataBytes( int ofsToName, int fnPointerSize, int fnNum, int offset )
{
	std::vector< char > bytes;

	int pos = fnPointerSize + (fnNum * 4);
	//Todo: Calculate this correctly
	pos -= myScript->GetFnDataOfs();

	char *ofsBytes = IntToBytes( pos );

	//Offset 1
	bytes.push_back( ofsBytes[0] );
	bytes.push_back( ofsBytes[1] );
	bytes.push_back( ofsBytes[2] );
	bytes.push_back( ofsBytes[3] );

	free( ofsBytes );

	//Offset to function name string
	char *seg = IntToBytes( ofsToName );
	for( int i = 0; i < 4; i++ )
	{
		bytes.push_back( seg[i] );
	}
	free( seg );

	//Offset 3
	bytes.push_back( 0x00 );
	bytes.push_back( 0x00 );
	bytes.push_back( 0x00 );
	bytes.push_back( 0x00 );

	//Data
	bytes.push_back( 0x00 );
	bytes.push_back( 0x00 );
	bytes.push_back( 0x00 );
	bytes.push_back( 0x00 );

	return bytes;
}