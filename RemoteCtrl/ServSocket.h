#pragma once
#include "pch.h"
#include "framework.h"

class CServSocket
{
public:
	// 获取全局唯一的服务端 socket 管理对象。
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

		// 先把地址结构体清零，避免里面残留随机值影响 bind。
		memset(&serv_adr, 0, sizeof(serv_adr));

		// AF_INET 表示使用 IPv4 地址。
		serv_adr.sin_family = AF_INET;

		// INADDR_ANY 表示监听本机所有网卡 IP。
		// 比如本机有 127.0.0.1、局域网 IP，都可以接收连接。
		serv_adr.sin_addr.s_addr = INADDR_ANY;

		// htons 用来把主机字节序转换成网络字节序。
		// 这里表示服务端监听 9527 端口。
		serv_adr.sin_port = htons(9527);

		if (bind(m_ServSock, (const sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		{
			return false;
		}

		// listen 后，这个 socket 才真正变成“监听 socket”。
		// 第二个参数 1 表示连接等待队列最多放 1 个客户端。
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
		int cli_sz = sizeof(client_adr);

		// accept 成功后返回一个新的 socket。
		// m_ServSock 继续负责监听，m_client 负责和这个客户端通信。
		m_client = accept(m_ServSock, (sockaddr*)&client_adr, &cli_sz);
		if (m_client == -1)
		{
			return false;
		}

		char buffer[1024];
		//recv(m_ServSock, buffer, sizeof(buffer), 0);
		//send(m_ServSock, buffer, sizeof(buffer), 0);

		return true;
	}

	// 处理客户端发来的命令。
	// 当前代码只负责循环接收数据，具体“命令解析和执行”还在 TODO 中。
	int dealCommand()
	{
		if (m_client == -1)
		{
			return false;
		}

		char buffer[1024] = "";
		while (true)
		{
			// recv 会从客户端 socket 读取数据。
			// 返回值 > 0：实际收到的字节数
			// 返回值 = 0：客户端正常关闭连接
			// 返回值 < 0：接收失败
			int len = recv(m_client, buffer, sizeof(buffer), 0);
			if (len <= 0)
			{
				return -1;
			}
			//TODO：处理命令
		}
	}

	// 向当前已连接的客户端发送数据。
	bool bSend(const char* pData, int nSize)
	{
		return send(m_client, pData, nSize, 0) > 0;
	}

private:
	// m_client：和某一个客户端通信用的 socket，由 accept 返回。
	SOCKET m_client;

	// m_ServSock：服务端监听用的 socket，只负责 bind/listen/accept。
	SOCKET m_ServSock;

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

		// 创建 TCP socket。
		// PF_INET 表示 IPv4，SOCK_STREAM 表示 TCP。
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

