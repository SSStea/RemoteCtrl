#include "pch.h"
#include "EdoyunTool.h"

void CEdoyunTool::Dump(BYTE* pData, size_t nSize)
{
		std::string strOut;
		for (size_t i = 0; i < nSize; i++)
		{
			char buf[8] = "";
			if (i > 0 && (i % 16 == 0))
			{
				strOut += "\n";
			}
			snprintf(buf, sizeof(buf), "%02X", pData[i] & 0xFF);
			strOut += buf;
		}
		strOut += "\n";
		OutputDebugStringA(strOut.c_str());
	}

int CEdoyunTool::nBytes2Image(CImage& image, const std::string& strBuffer)
{
		BYTE* pData = (BYTE*)strBuffer.c_str();
		IStream* pStream = NULL;
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
		if (hMem == NULL)
		{
			TRACE("内存不足！！");
			Sleep(1);
			return -1;
		}
		HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
		if (hRet == S_OK)
		{
			ULONG  ulLength = 0;
			hRet = pStream->Write(pData, (ULONG)strBuffer.size(), &ulLength);
			if (hRet != S_OK)
			{
				TRACE("图像写入流失败！！！\r\n");
				return hRet;
			}
			LARGE_INTEGER begin = { 0 };
			hRet = pStream->Seek(begin, STREAM_SEEK_SET, NULL);
			if (hRet != S_OK)
			{
				TRACE("STREAM_SEEK_SET 失败！！！\r\n");
				return hRet;
			}
			if ((HBITMAP)image != NULL)
			{
				image.Destroy();
			}
			hRet = image.Load(pStream);
		}

		pStream->Release();
		return hRet;
	}
