#pragma once
#include <cstddef>
#include <cstring>
#include <iostream>
using namespace std;

const unsigned long DeFrameLen = 82500;		// 1500 * 55
const unsigned long DefIspLen = 1024;
const unsigned long DefInc = 1500;

template<class T, unsigned long ulDefCap = DeFrameLen, unsigned long ulDefInc = DefInc>
class DataBuf
{
	T* data;
	unsigned long	ulCapacity;
	unsigned long	ulCount;
public:
	DataBuf()
	{
		ulCount = 0;
		data = new T[ulCapacity = ulDefCap];
	}
	
	~DataBuf()
	{
		delete [] data;
	}
	
	void Push(const T& t)
	{
		// Inc DataBuf
		if (ulCount >= ulCapacity)
		{
			unsigned long ulNewCap = ulCapacity + ulDefInc;
			T* newData = new T[ulNewCap];
			memcpy(newData, data, ulCount*sizeof(T));
			delete [] data;
			data = newData;
			ulCapacity = ulNewCap;
		}
		else
		{
			data[ulCount] = t;
			ulCount++;
		}
	}

	void Reset()
	{
		memset(data, 0x00, sizeof(T)*ulCount);
		ulCount = 0;
	}

	unsigned long Length()
	{
		return ulCount;
	}

	T* Begin()
	{
		return data;
	}
};



