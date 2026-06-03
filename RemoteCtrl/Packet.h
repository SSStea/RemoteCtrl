#pragma once
#include "pch.h"
#include "framework.h"
#pragma pack(push)
#pragma pack(1)
class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}

	CPacket(const CPacket& packet)
	{
		sHead = packet.sHead;
		nLength = packet.nLength;
		sCmd = packet.sCmd;
		strData = packet.strData;
		sSum = packet.sSum;
	}

	CPacket& operator=(const CPacket& packet)
	{
		if (this == &packet)
		{
			return *this;
		}
		sHead = packet.sHead;
		nLength = packet.nLength;
		sCmd = packet.sCmd;
		strData = packet.strData;
		sSum = packet.sSum;
	}

	//解析包的构造函数
	CPacket(const BYTE* pData, size_t& nSize)
	{
		size_t pos = 0;//代表目前数据解析到哪个位置
		for (; pos < nSize; pos++)
		{
			if (*(WORD*)(pData + pos) == 0xFEFF)
			{
				sHead = *(WORD*)(pData + pos);
				pos += 2;//解析完包头，位置到包头之后
				break;
			}
		}
		if (pos + 8 > nSize)//4：nLength，2：sCmd，2：sSum，后面就不会访问越界
		{//包数据可能不全，或者包头未能全部接收到
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + pos);
		pos += 4;//解析完长度，位置到长度之后
		if (nLength + pos > nSize)
		{//包未完全接收到，就返回，解析失败
			nSize = 0;
			return;
		}

		sCmd = *(WORD*)(pData + pos);
		pos += 2;//解析完控制命令，位置到命令之后

		if (nLength > 4)
		{
			strData.resize(nLength - 2 - 2);//减掉sCmd和sSum的长度
			memcpy((void*)strData.c_str(), pData + pos, nLength - 4);
			pos += (nLength - 4);//解析完数据，位置到数据之后
		}

		sSum = *(WORD*)(pData + pos);
		pos += 2;//解析完校验，位置到校验之后
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum)
		{
			nSize = pos;
			return;
		}
		nSize = 0;
	}

	//构造包的构造函数
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
	{
		sHead = 0xFEFF;
		nLength = (DWORD)nSize + 4;//数据长度+命令长度+校验长度
		sCmd = nCmd;

		if (nSize > 0)
		{
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else
		{
			strData.clear();
		}

		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}

	//获取包的大小
	int Size()
	{
		return nLength + 6;
	}

	//获取包的数据
	const char* Data()
	{
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead;
		pData += 2;

		*(DWORD*)pData = nLength;
		pData += 4;

		*(WORD*)pData = sCmd;
		pData += 2;

		memcpy(pData, strData.c_str(), strData.size());
		pData += strData.size();

		*(WORD*)pData = sSum;

		return strOut.c_str();
	}

	~CPacket()
	{
	}
public:
	WORD		sHead;		//包头：固定FE FF
	DWORD		nLength;	//包长度：从控制命令->校验
	WORD		sCmd;		//控制命令
	std::string strData;	//包数据
	WORD		sSum;		//校验
	std::string strOut;		//整个包的数据
};
#pragma pack(pop)

typedef struct MouseEvent
{
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD	nAction;	//点击 移动 双击
	WORD	nButton;	//左键 右键 中键
	POINT	ptXY;		//坐标
}MOUSEEVENT, * pMOUSEEVENT;

typedef struct file_info
{
	file_info()
	{
		bIsInvalid = FALSE;
		bIsDirectory = -1;
		bHasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL bIsInvalid;            //是否无效：0否 1是
	BOOL bIsDirectory;          //是否为目录：0否 1是
	BOOL bHasNext;              //是否还有下一个文件：0无 1有
	char szFileName[256];       //文件名
}FILEINFO, * pFILEINFO;