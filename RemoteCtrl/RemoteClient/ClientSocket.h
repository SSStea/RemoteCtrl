#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <list>
#include <map>

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
		hEvent = packet.hEvent;
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
		hEvent = packet.hEvent;
	}

	//解析包的构造函数
	CPacket(const BYTE* pData, size_t& nSize):hEvent(INVALID_HANDLE_VALUE)
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
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize, HANDLE hEvent)
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

		this->hEvent = hEvent;
	}

	//获取包的大小
	int Size()
	{
		return nLength + 6;
	}

	//获取包的数据
	const char* Data(std::string& strOut) const
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
	HANDLE		hEvent;
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
	WORD	nAction;	//点击0 移动1 双击2
	WORD	nButton;	//左键0 右键1 中键2
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

std::string GetSockErrInfo(int wsaErrcode);

class CClientSocket
{
public:
	// 获取全局唯一的服务端 socket 管理对象，初始化Windows socket环境
	// 第一次调用时会 new 一个对象，后续调用都返回同一个对象。
	static CClientSocket* getInstance()
	{//静态函数没有this指针，无法访问成员变量，只能将成员变量声明为静态
		if (m_Instance == NULL)
		{
			m_Instance = new CClientSocket();
		}
		return m_Instance;
	}

	// 初始化客户端连接 socket：
	// 1. 准备服务器地址
	// 2. 客户端连接服务器
	bool bInitSocket()
	{
		if (m_Sock != INVALID_SOCKET)
		{
			CloseSocket();
		}
		m_Sock = socket(PF_INET, SOCK_STREAM, 0);

		if (m_Sock == -1)
		{
			return false;
		}

		sockaddr_in serv_adr;

		memset(&serv_adr, 0, sizeof(serv_adr));

		serv_adr.sin_family = AF_INET;
		TRACE("addr %08X nIP %08X\r\n", inet_addr("127.0.0.1"), m_nIP);
		serv_adr.sin_addr.s_addr = htonl(m_nIP);
		serv_adr.sin_port = htons(m_nPort);

		if (serv_adr.sin_addr.s_addr == INADDR_NONE)
		{
			AfxMessageBox("指定IP地址不存在！");
			return false;
		}

		int ret = connect(m_Sock, (const sockaddr*)&serv_adr, sizeof(serv_adr));
		if (ret == -1)
		{
			AfxMessageBox("连接失败！");
			TRACE("连接失败：%d %s\r\n", WSAGetLastError(), 
				GetSockErrInfo(WSAGetLastError()).c_str());
			return false;
		}

		return true;
	}

#define BUFFER_SIZE 20480000
	// 处理客户端发来的命令。
	int dealCommand()
	{
		if (m_Sock == -1)
		{
			return -1;
		}

		char* buffer = m_vecBuffer.data();
		static size_t index = 0;
		//定义为静态全局变量：服务端可能一次性发来多个包的数据，底下的处理逻辑是每解析一个包就返回，
		//	并将处理完的数据从buffer中移除、修改index的索引值，因此index不能在每次处理命令时都为0，
		//	这样会导致每次存储数据都从0开始存储，从而覆盖之前服务端发来的但没有处理完的数据
		//指向当前buffer存储的数据的位置，值表示当前存储的总长度
		while (true)
		{
			size_t len = recv(m_Sock, buffer + index, BUFFER_SIZE - (int)index, 0);
			if ((int)len <= 0 && (int)index <= 0)//表示没读到并且缓冲区里没数据
			{
				return -1;
			}
			TRACE("recv len = %d(0x%08X) index = %d(0x%08X)\r\n", len, len, index, index);
			index += len;//更新数据在buffer中的存储索引值index：将recv的数据长度len加到上一次的index
			len = index;//将buffer存储的数据长度改为当前buffer存储数据的索引位置
			TRACE("recv len = %d(0x%08X) index = %d(0x%08X)\r\n", len, len, index, index);
			m_packet = CPacket((BYTE*)buffer, len);
			TRACE("cmd = %d\r\n", m_packet.sCmd);
			//按引用传入当前数据的长度len，将buffer解析，将数据封装为Packet并返回封装了的数据的长度len
			if (len > 0)//如果解析到了数据
			{
				memmove(buffer, buffer + len, index - len);//将解析到的数据从buffer中移走
				index -= len;//变更数据在buffer中的存储索引值index，减掉解析到的数据长度len
				return m_packet.sCmd;
			}
		}
		return -1;
	}

	// 向当前已连接的客户端发送数据。
	bool bSend(const char* pData, int nSize)
	{
		if (m_Sock == -1)
		{
			return false;
		}
		return send(m_Sock, pData, nSize, 0) > 0;
	}
	bool bSend(const CPacket& pack)
	{
		TRACE("m_Sock = %d\r\n", m_Sock);
		if (m_Sock == -1)
		{
			return false;
		}

		std::string strOut;
		pack.Data(strOut);
		return send(m_Sock, strOut.c_str(), (int)strOut.size(), 0) > 0;
	}

	bool bGetFilePath(std::string& strPath)
	{
		if (2 <= m_packet.sCmd && m_packet.sCmd <= 4)
		{
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}

	bool bGetMouseEvent(MOUSEEVENT& mouse)
	{
		if (m_packet.sCmd == 5)
		{
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEVENT));
			return true;
		}
		return false;
	}

	CPacket& getPacket()
	{
		return m_packet;
	}

	void CloseSocket()
	{
		closesocket(m_Sock);
		m_Sock = INVALID_SOCKET;
	}

	void UpdataAddress(int nIP, int nPort)
	{
		m_nIP = nIP;
		m_nPort = nPort;
	}

private:
	SOCKET									m_Sock;
	CPacket									m_packet;
	std::vector<char>						m_vecBuffer;
	int										m_nIP;
	int										m_nPort;
	std::map<HANDLE, std::list<CPacket>>	m_mapAck;//处理来自服务端的包的映射
	std::list<CPacket>						m_lstSendPkt;//要发送的数据

	// 构造函数私有化，是单例模式的关键：
	// 外部不能直接 new CServSocket，只能通过 getInstance 获取唯一对象。
	CClientSocket() : m_nIP(INADDR_ANY), m_nPort(0)
	{
		// Windows 下使用 socket 前，必须先调用 WSAStartup 初始化 Winsock 环境。
		if (!bInitSockEnv())
		{
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_vecBuffer.resize(BUFFER_SIZE);
		memset(m_vecBuffer.data(), 0, BUFFER_SIZE);
	}

	// 拷贝构造和赋值运算符放在 private 中，目的是禁止外部复制单例对象。
	CClientSocket(const CClientSocket& ss) 
	{
		m_Sock = ss.m_Sock;
		m_nIP = ss.m_nIP;
		m_nPort = ss.m_nPort;
	}
	CClientSocket& operator=(const CClientSocket& ss){}

	~CClientSocket()
	{
		// 关闭监听 socket，并释放 Winsock 环境。
		closesocket(m_Sock);
		WSACleanup();
	}

	static void threadPktHandleEntry(void* arg);
	void threadPktHandle();

	// 初始化 Windows socket 环境。
	// WSAStartup 成功后，后面的 socket/bind/listen/accept 才能正常使用。
	BOOL bInitSockEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)//TODO：返回值处理
		{
			return FALSE;
		}
		return TRUE;
	}

	// 释放单例对象。
	// delete 会触发析构函数，从而 closesocket 和 WSACleanup。
	static void releaseInstance()
	{
		if (m_Instance != NULL)
		{
			CClientSocket* tmp = m_Instance;
			m_Instance = NULL;
			delete tmp;
		}
	}

	// 保存全局唯一的 CClientSocket 对象地址。
	static CClientSocket* m_Instance;
	class CHelper
	{
	public:
		// 程序启动时，静态成员 m_helper 会先构造。
		// 这里主动调用 getInstance，让 socket 管理对象提前创建。
		CHelper()
		{
			CClientSocket::getInstance();
		}

		// 程序结束时，静态成员 m_helper 会析构。
		// 这里释放单例对象，完成资源清理。
		~CHelper()
		{
			CClientSocket::releaseInstance();
		}
	};

	// 辅助释放单例的静态对象。
	static CHelper m_helper;
};

