#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <list>
#include <map>
#include <mutex>

#define WM_SEND_PACK	(WM_USER+1)//发送包数据
#define WM_SEND_ACK		(WM_USER+2)//发送应答包

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
	const char* Data(std::string& strOut) const;

	~CPacket();
public:
	WORD		sHead;		//包头：固定FE FF
	DWORD		nLength;	//包长度：从控制命令->校验
	WORD		sCmd;		//控制命令
	std::string strData;	//包数据
	WORD		sSum;		//校验
};
#pragma pack(pop)

typedef struct MouseEvent
{
	MouseEvent();
	WORD	nAction;	//点击0 移动1 双击2
	WORD	nButton;	//左键0 右键1 中键2
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

typedef struct PacketData
{
	std::string strData;
	UINT		nMode;
	LPARAM      lParam;
	PacketData(const char* pData, size_t nLen, UINT mode, LPARAM nParam = 0);

	PacketData(const PacketData& data);

	PacketData& operator=(const PacketData& data);
}PACKETDATA;

enum {
	CSM_AUTOCLOSE = 1 // CSM = Client Socket Mode 自动关闭模式

};

std::string GetSockErrInfo(int wsaErrcode);

#define BUFFER_SIZE 20480000
class CClientSocket
{
public:
	// 获取全局唯一的服务端 socket 管理对象，初始化Windows socket环境
	// 第一次调用时会 new 一个对象，后续调用都返回同一个对象。
	static CClientSocket* getInstance();

	// 初始化客户端连接 socket：
	// 1. 准备服务器地址
	// 2. 客户端连接服务器
	bool bInitSocket();

	bool bSendPkt(HWND hWnd, const CPacket& reqPkt, bool bIsAutoClosed = true, LPARAM lParam = 0);

	bool bGetFilePath(std::string& strPath);

	bool bGetMouseEvent(MOUSEEVENT& mouse);

	void CloseSocket();

	void UpdataAddress(int nIP, int nPort);

private:
	SOCKET					m_Sock;
	CPacket					m_packet;
	std::vector<char>		m_vecBuffer;
	int						m_nIP;
	int						m_nPort;
	bool					m_bAutoClosed;
	HANDLE					m_hPktThread;
	UINT					m_hPktThreadID;
	HANDLE					m_hPktThreadReadyEvt;

	typedef void(CClientSocket::* PKTFUNC)(UINT, WPARAM, LPARAM);
	std::map<UINT, PKTFUNC> m_mapPktFunc;

	// 构造函数私有化，是单例模式的关键：
	// 外部不能直接 new CServSocket，只能通过 getInstance 获取唯一对象。
	CClientSocket();

	// 拷贝构造和赋值运算符放在 private 中，目的是禁止外部复制单例对象。
	CClientSocket(const CClientSocket& ss);
	CClientSocket& operator=(const CClientSocket& ss);

	~CClientSocket();

	static unsigned __stdcall threadPktHandleEntry(void* arg);
	void threadPktHandle2();

	// 初始化 Windows socket 环境。
	// WSAStartup 成功后，后面的 socket/bind/listen/accept 才能正常使用。
	BOOL bInitSockEnv();

	// 释放单例对象。
	// delete 会触发析构函数，从而 closesocket 和 WSACleanup。
	static void releaseInstance();


	// 向当前已连接的客户端发送数据。
	bool bSend(const char* pData, int nSize);
	bool bSend(const CPacket& pack);

	void sendPack(UINT nMsg, WPARAM wParam/*缓冲区的值*/, LPARAM lParam/*缓冲区长度*/);

	// 保存全局唯一的 CClientSocket 对象地址。
	static CClientSocket* m_Instance;
	class CHelper
	{
	public:
		// 程序启动时，静态成员 m_helper 会先构造。
		// 这里主动调用 getInstance，让 socket 管理对象提前创建。
		CHelper();

		// 程序结束时，静态成员 m_helper 会析构。
		// 这里释放单例对象，完成资源清理。
		~CHelper();
	};

	// 辅助释放单例的静态对象。
	static CHelper m_helper;
};

