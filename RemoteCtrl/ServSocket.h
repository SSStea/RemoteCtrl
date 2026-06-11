#pragma once
#include "pch.h"
#include "framework.h"
#include <list>
#include "Packet.h"

typedef void (*SOCKET_CALLBACK)(void*, int, std::list<CPacket>&, CPacket& );

class CServSocket
{
public:
	// 获取全局唯一的服务端 socket 管理对象，初始化Windows socket环境
	// 第一次调用时会 new 一个对象，后续调用都返回同一个对象。
	static CServSocket* getInstance();

	int nRun(SOCKET_CALLBACK callback, void* arg, short sPort = 9527);

protected:
	// 初始化服务端监听 socket：
	// 1. 准备服务器地址
	// 2. 绑定 IP 和端口
	// 3. 开始监听客户端连接
	bool bInitSocket(short sPort = 9527);

	// 等待客户端连接。
	// accept 是阻塞函数：没有客户端连上来时，程序会停在这里等待。
	bool bAcceptClient();
#define BUFFER_SIZE 4096
	// 处理客户端发来的命令。
	int dealCommand();

	// 向当前已连接的客户端发送数据。
	bool bSend(const char* pData, int nSize);
	bool bSend(CPacket& pack);

	void CloseClient();

private:
	SOCKET_CALLBACK m_callback;
	void* m_arg;
	SOCKET	m_client;
	SOCKET	m_ServSock;
	CPacket m_packet;

	// 构造函数私有化，是单例模式的关键：
	// 外部不能直接 new CServSocket，只能通过 getInstance 获取唯一对象。
	CServSocket();

	// 拷贝构造和赋值运算符放在 private 中，目的是禁止外部复制单例对象。
	CServSocket(const CServSocket& ss);
	CServSocket& operator=(const CServSocket& ss);

	~CServSocket();

	// 初始化 Windows socket 环境。
	// WSAStartup 成功后，后面的 socket/bind/listen/accept 才能正常使用。
	BOOL bInitSockEnv();

	// 释放单例对象。
	// delete 会触发析构函数，从而 closesocket 和 WSACleanup。
	static void releaseInstance();

	// 保存全局唯一的 CServSocket 对象地址。
	static CServSocket* m_Instance;
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

