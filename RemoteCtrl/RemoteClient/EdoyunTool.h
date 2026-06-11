#pragma once
#include <windows.h>
#include <atlimage.h>
#include <string>

class CEdoyunTool
{
public:
	static void Dump(BYTE* pData, size_t nSize);

	static int nBytes2Image(CImage& image, const std::string& strBuffer);
};

