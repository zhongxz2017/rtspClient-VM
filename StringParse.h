#pragma once
#include <string.h>
#include <malloc.h>

namespace atl
{
	class StringParse
	{
	public:
		StringParse(void);
		~StringParse(void);

		static bool IsNumericString(const char* str);

		static int Trim(char* str);
		static int Find(const char* str, char ch);
		static int Find(const char* str, char ch, char endFlag);
		static int Find(const char* str, const char* subStr, bool ignoreCase = true);

		static int SplitString(const char* str, char separator, char* substringArrayBuf, int arraySize, int arrayElemSize, int& elemNum);

		static int GetSubString(const char* str, unsigned int offset, char endFlag, char* buf, unsigned int bufLen, bool isTrim);
	

	};
}
