#pragma once
#include "pch.h"
#include "framework.h"

void Dump(BYTE* pData, size_t nSize);

#pragma pack(push)
#pragma pack(1)
class CPacket
{
public:
	CPacket():sHead(0), nLength(0), sCmd(0), sSum(0) {}

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

		if(nLength > 4)
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

		if(nSize > 0)
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
	{ }
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
		ptXY.x	= 0;
		ptXY.y	= 0;
	}
	WORD	nAction;	//点击 移动 双击
	WORD	nButton;	//左键 右键 中键
	POINT	ptXY;		//坐标
}MOUSEEVENT, *pMOUSEEVENT;

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

class CServSocket
{
public:
	// 获取全局唯一的服务端 socket 管理对象，初始化Windows socket环境
	// 第一次调用时会 new 一个对象，后续调用都返回同一个对象。
	static CServSocket* getInstance()
	{//静态函数没有this指针，无法访问成员变量，只能将成员变量声明为静态
		if (m_Instance == NULL)
		{
			m_Instance = new CServSocket();
		}
		return m_Instance;
	}

	// 初始化服务端监听 socket：
	// 1. 准备服务器地址
	// 2. 绑定 IP 和端口
	// 3. 开始监听客户端连接
	bool bInitSocket()
	{
		if (m_ServSock == -1)
		{
			return false;
		}

		sockaddr_in serv_adr;

		memset(&serv_adr, 0, sizeof(serv_adr));

		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(9527);

		if (bind(m_ServSock, (const sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		{
			return false;
		}

		if (listen(m_ServSock, 1) == -1)
		{
			return false;
		}

		return true;
	}

	// 等待客户端连接。
	// accept 是阻塞函数：没有客户端连上来时，程序会停在这里等待。
	bool bAcceptClient()
	{
		sockaddr_in client_adr;
		int			cli_sz = sizeof(client_adr);

		m_client = accept(m_ServSock, (sockaddr*)&client_adr, &cli_sz);
		TRACE("m_client = %d\r\n", m_client);
		if (m_client == -1)
		{
			return false;
		}

		return true;
	}
#define BUFFER_SIZE 4096
	// 处理客户端发来的命令。
	int dealCommand()
	{
		if (m_client == -1)
		{
			return -1;
		}

		char* buffer = new char[BUFFER_SIZE];
		if (buffer == NULL)
		{
			TRACE("内存不足");
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;//指向当前buffer存储的数据的位置，值表示当前存储的总长度
		while (true)
		{
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - (int)index, 0);
			if (len <= 0)
			{
				delete []buffer;
				return -1;
			}
			TRACE("recv = %d\r\n", len);
			index += len;//收到了数据更新位置，下次再收到数据从index开始存储
			len = index;//将长度改为当前buffer的总长度
			m_packet = CPacket((BYTE*)buffer, len);//将buffer解析，得到解析后的数据和长度组包
			if (len > 0)//如果解析到了数据
			{
				memmove(buffer, buffer + len, BUFFER_SIZE - len);//将解析到的数据从buffer中移走
				index -= len;//总长度减掉解析的数据长度
				delete []buffer;
				return m_packet.sCmd;
			}
		}
		delete []buffer;
		return -1;
	}

	// 向当前已连接的客户端发送数据。
	bool bSend(const char* pData, int nSize)
	{
		if (m_client == -1)
		{
			return false;
		}
		return send(m_client, pData, nSize, 0) > 0;
	}
	bool bSend(CPacket& pack)
	{
		if (m_client == -1)
		{
			return false;
		}
		//Dump((BYTE*)pack.Data(), pack.Size());
		Sleep(1);//如果不做处理可能由于发送太快导致客户端缓冲区满了被覆盖从而丢包，
					//进而导致客户端显示文件、目录信息不全，因此这里延迟1ms
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}

	bool bGetFilePath(std::string& strPath)
	{
		if (2 <= m_packet.sCmd  && m_packet.sCmd <= 4 || m_packet.sCmd == 9)
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

	void CloseClient()
	{
		closesocket(m_client);
		m_client = INVALID_SOCKET;
	}

private:
	SOCKET	m_client;
	SOCKET	m_ServSock;
	CPacket m_packet;

	// 构造函数私有化，是单例模式的关键：
	// 外部不能直接 new CServSocket，只能通过 getInstance 获取唯一对象。
	CServSocket()
	{
		m_client = INVALID_SOCKET;

		// Windows 下使用 socket 前，必须先调用 WSAStartup 初始化 Winsock 环境。
		if (!bInitSockEnv())
		{
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}

		m_ServSock = socket(PF_INET, SOCK_STREAM, 0);
	}

	// 拷贝构造和赋值运算符放在 private 中，目的是禁止外部复制单例对象。
	CServSocket(const CServSocket&) {}
	CServSocket& operator=(const CServSocket& ss)
	{
		m_ServSock = ss.m_ServSock;
		m_client = ss.m_client;
		return *this;
	}

	~CServSocket()
	{
		// 关闭监听 socket，并释放 Winsock 环境。
		closesocket(m_ServSock);
		WSACleanup();
	}

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
			CServSocket* tmp = m_Instance;
			m_Instance = NULL;
			delete tmp;
		}
	}

	// 保存全局唯一的 CServSocket 对象地址。
	static CServSocket* m_Instance;
	class CHelper 
	{
	public:
		// 程序启动时，静态成员 m_helper 会先构造。
		// 这里主动调用 getInstance，让 socket 管理对象提前创建。
		CHelper()
		{
			CServSocket::getInstance();
		}

		// 程序结束时，静态成员 m_helper 会析构。
		// 这里释放单例对象，完成资源清理。
		~CHelper()
		{
			CServSocket::releaseInstance();
		}
	};

	// 辅助释放单例的静态对象。
	static CHelper m_helper;
};

