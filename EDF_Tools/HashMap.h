#pragma once
#include <string>
#include <vector>
#include <unordered_map>

typedef std::unordered_map<std::string, int> StringToIntMap;
typedef std::unordered_map<std::wstring, int> WStringToIntMap;

typedef struct DataRelativeOffset_t {
	int WritePos; // where to write the offset
	int BasePos; // where to calculate the offset from
}*PDataRelativeOffset;

typedef struct RelativeOffsetSet_Base_t {
	std::vector<DataRelativeOffset_t> v_offset;
}*PRelativeOffsetSet_Base;

typedef struct RelativeOffsetSet_WString_t : RelativeOffsetSet_Base_t {
	std::wstring wstr;
}*PRelativeOffsetSet_WString;

typedef struct RelativeOffsetSet_Data_t : RelativeOffsetSet_Base_t {
	std::vector<char> data;
	size_t size; // because the data needs to be aligned to 16 bytes, but this is not its actual size.
}*PRelativeOffsetSet_Data;
