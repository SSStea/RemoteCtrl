#pragma once
#include "pch.h"
#include "framework.h"
#pragma pack(push)
#pragma pack(1)
class CPacket
{
public:
	CPacket();

	CPacket(const CPacket& packet);

	CPacket& operator=(const CPacket& packet);

	//解析包的构造函数
	CPacket(const BYTE* pData, size_t& nSize);

	//构造包的构造函数
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize);

	//获取包的大小
	int Size();

	//获取包的数据
	const char* Data();

	~CPacket();
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
	MouseEvent();
	WORD	nAction;	//点击 移动 双击
	WORD	nButton;	//左键 右键 中键
	POINT	ptXY;		//坐标
}MOUSEEVENT, * pMOUSEEVENT;

typedef struct file_info
{
	file_info();
	BOOL bIsInvalid;            //是否无效：0否 1是
	BOOL bIsDirectory;          //是否为目录：0否 1是
	BOOL bHasNext;              //是否还有下一个文件：0无 1有
	char szFileName[256];       //文件名
}FILEINFO, * pFILEINFO;