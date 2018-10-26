
#include "StringParse.h"


namespace atl
{

StringParse::StringParse(void)
{
}

StringParse::~StringParse(void)
{
}

bool StringParse::IsNumericString(const char* str)
{
	for(int i=0; str[i]!='\0'; ++i)
	{
		if(str[i] < '0' || str[i] > '9')
			return false;
	}

	return true;
}


int StringParse::Trim(char* str)
{
	int validIndex = 0;
	int lastNotSpaceIndex = -1;

	for(int i=0; str[i]!='\0'; ++i)
	{
		if(str[i] != ' ' || validIndex > 0)
		{
			if(i != validIndex)
				str[validIndex] = str[i];

			if(str[i] != ' ')
				lastNotSpaceIndex = validIndex;

			++validIndex;
		}
	}
	
	if(str[lastNotSpaceIndex+1] != '\0')
		str[lastNotSpaceIndex+1] = '\0';

	return 0;
}

int StringParse::Find(const char* str, char ch)
{
	int index = -1;
	for(int i=0; str[i]!='\0'; ++i)
	{
		if(str[i] == ch)
		{
			index = i;
			break;
		}
	}

	return index;
}

int StringParse::Find(const char* str, char ch, char endFlag)
{
	int index = -1;
	for(int i=0; str[i]!='\0' && str[i]!=endFlag; ++i)
	{
		if(str[i] == ch)
		{
			index = i;
			break;
		}
	}

	return index;
}

int StringParse::Find(const char* str, const char* subStr, bool ignoreCase)
{
	int index = -1;
	int len = strlen(str);
	int subStrLen = strlen(subStr);

	for(int i=0; i+subStrLen<=len; ++i)
	{
		if(ignoreCase)
		{
			if(strncasecmp(str+i, subStr, subStrLen) == 0)
			{
				index = i;
				break;
			}
		}
		else
		{
			if(strncmp(str+i, subStr, subStrLen) == 0)
			{
				index = i;
				break;
			}
		}
	}

	return index;
}

int StringParse::SplitString(const char* str, char separator, char* substringArrayBuf, int arraySize, int arrayElemSize, int& elemNum)
{
	int count = 0;
	int startIndex = 0;
	int nTemp;

	for(int i=0; str[i]!='\0'; ++i)
	{
		if(str[i] == separator)
		{
			nTemp = i-startIndex;
			if(nTemp >= arrayElemSize)
				return -1;

			if(nTemp > 0)
			{
				if(count >= arraySize)
					return -1;

				strncpy(substringArrayBuf+arrayElemSize*count, str+startIndex, nTemp);
				count++;
			}

			startIndex = i+1;
		}
	}

	int totalLen = strlen(str);
	if(startIndex < totalLen)
	{
		nTemp = totalLen-startIndex;
		if(nTemp >= arrayElemSize)
			return -1;

		if(count >= arraySize)
			return -1;

		strncpy(substringArrayBuf+arrayElemSize*count, str+startIndex, nTemp);
		count++;
	}

	elemNum = count;
	return 0;
}

int StringParse::GetSubString(const char* str, unsigned int offset, char endFlag, char* buf, unsigned int bufLen, bool isTrim)
{
	int endIndex = 0;
	int count = 0;

	for(int i=offset; str[i]!='\0' && str[i] != endFlag; ++i)
	{
		if(isTrim)
		{
			if(str[i] != ' ' || count > 0)
			{
				if(count>=bufLen-1)
					return -1;

				buf[count++] = str[i];
				if(str[i] != ' ')
					endIndex = count;
			}
		}
		else
		{
			if(count>=bufLen-1)
				return -1;

			buf[count++] = str[i];
		}
	}

	if(isTrim)
		buf[endIndex] = '\0';
	else
		buf[count] = '\0';

	return 0;

}

}