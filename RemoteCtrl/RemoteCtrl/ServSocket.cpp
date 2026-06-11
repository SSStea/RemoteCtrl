#include "pch.h"
#include "ServSocket.h"

// 单例对象指针的静态成员定义。
// 这里只是初始化为空，真正创建对象发生在 getInstance 里。
CServSocket* CServSocket::m_Instance = NULL;

// 静态辅助对象。
// 它的构造/析构会分别触发单例创建和释放。
CServSocket::CHelper CServSocket::m_helper;

// 如果打开这一行，全局变量初始化时也会主动创建服务端单例。
// 现在已经有 m_helper 负责创建和释放，所以这里暂时不需要。
//CServSocket* pServer = CServSocket::getInstance();

CServSocket::CHelper::CHelper()
{
			CServSocket::getInstance();
		}

CServSocket::CHelper::~CHelper()
{
			CServSocket::releaseInstance();
		}

CServSocket* CServSocket::getInstance()
{//静态函数没有this指针，无法访问成员变量，只能将成员变量声明为静态
		if (m_Instance == NULL)
		{
			m_Instance = new CServSocket();
		}
		return m_Instance;
	}

int CServSocket::nRun(SOCKET_CALLBACK callback, void* arg, short sPort)
{
		// 初始化监听 socket：创建地址、绑定 9527 端口、进入监听状态。
		bool bRet = bInitSocket(sPort);
		if (!bRet)
		{
			return -1;
		}

		std::list<CPacket> lstSendPacket;

		m_callback = callback;
		m_arg = arg;

		// 统计 accept 客户端失败的次数，连续失败太多就结束程序
		int nCount = 0;

		// 服务端主循环。
		// 只要单例对象还存在，就不断等待客户端接入并处理客户端命令。
		while (getInstance() != NULL)
		{
			// 等待客户端连接。
			// 如果没有客户端连接，bAcceptClient 内部的 accept 会阻塞等待。
			if (!bAcceptClient())
			{
				if (nCount >= 3)
				{
					return -2;
				}
				nCount++;
			}
			TRACE("Accept Client return true\r\n");

			// 客户端连接成功后，进入命令处理逻辑。
			// 当前 dealCommand 里还没有真正解析命令，只是在循环 recv
			int nRetCmd = dealCommand();
			TRACE("deal commmand nRet = %d\r\n", nRetCmd);
			if (nRetCmd > 0)
			{
				//解析到的命令通过回调函数执行
				m_callback(m_arg, nRetCmd, lstSendPacket, m_packet);
				while (lstSendPacket.size() > 0)
				{
					bSend(lstSendPacket.front());
					lstSendPacket.pop_front();
				}
			}
			CloseClient();
		}

		return 0;
	}

bool CServSocket::bInitSocket(short sPort)
{
		if (m_ServSock == -1)
		{
			return false;
		}

		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(sPort);

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

bool CServSocket::bAcceptClient()
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
	int CServSocket::dealCommand()
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

bool CServSocket::bSend(const char* pData, int nSize)
{
		if (m_client == -1)
		{
			return false;
		}
		return send(m_client, pData, nSize, 0) > 0;
	}

bool CServSocket::bSend(CPacket& pack)
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

void CServSocket::CloseClient()
{
		if (m_client != INVALID_SOCKET)
		{
			closesocket(m_client);
			m_client = INVALID_SOCKET;
		}
	}

CServSocket::CServSocket()
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

CServSocket::CServSocket(const CServSocket& ss)
{
		m_ServSock = ss.m_ServSock;
		m_client = ss.m_client;
	}

#pragma warning(push)
#pragma warning(disable: 4716)
CServSocket& CServSocket::operator=(const CServSocket& ss)
{}
#pragma warning(pop)

CServSocket::~CServSocket()
{
		// 关闭监听 socket，并释放 Winsock 环境。
		closesocket(m_ServSock);
		WSACleanup();
	}

BOOL CServSocket::bInitSockEnv()
{
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)//TODO：返回值处理
		{
			return FALSE;
		}
		return TRUE;
	}

void CServSocket::releaseInstance()
{
		if (m_Instance != NULL)
		{
			CServSocket* tmp = m_Instance;
			m_Instance = NULL;
			delete tmp;
		}
	}
