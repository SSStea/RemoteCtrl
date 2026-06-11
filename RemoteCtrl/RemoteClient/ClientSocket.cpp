#include "pch.h"
#include "ClientSocket.h"

// 单例对象指针的静态成员定义。
// 这里只是初始化为空，真正创建对象发生在 getInstance 里。
CClientSocket* CClientSocket::m_Instance = NULL;

// 静态辅助对象。
// 它的构造/析构会分别触发单例创建和释放。
CClientSocket::CHelper CClientSocket::m_helper;

// 如果打开这一行，全局变量初始化时也会主动创建服务端单例。
// 现在已经有 m_helper 负责创建和释放，所以这里暂时不需要。
//CServSocket* pServer = CServSocket::getInstance();

CClientSocket::CHelper::CHelper()
{
			CClientSocket::getInstance();
		}

CClientSocket::CHelper::~CHelper()
{
			CClientSocket::releaseInstance();
		}

CClientSocket* CClientSocket::getInstance()
{//静态函数没有this指针，无法访问成员变量，只能将成员变量声明为静态
		if (m_Instance == NULL)
		{
			m_Instance = new CClientSocket();
		}
		return m_Instance;
	}

bool CClientSocket::bInitSocket()
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

bool CClientSocket::bGetFilePath(std::string& strPath)
{
		if (2 <= m_packet.sCmd && m_packet.sCmd <= 4)
		{
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}

bool CClientSocket::bGetMouseEvent(MOUSEEVENT& mouse)
{
		if (m_packet.sCmd == 5)
		{
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEVENT));
			return true;
		}
		return false;
	}

void CClientSocket::CloseSocket()
{
		closesocket(m_Sock);
		m_Sock = INVALID_SOCKET;
	}

void CClientSocket::UpdataAddress(int nIP, int nPort)
{
		if(m_nIP != nIP || m_nPort != nPort)
		{
			m_nIP = nIP;
			m_nPort = nPort;
		}
	}

CClientSocket::CClientSocket() : m_nIP(INADDR_ANY), m_nPort(0), m_Sock(INVALID_SOCKET), 
		m_bAutoClosed(true), m_hPktThread(INVALID_HANDLE_VALUE)
{
		// Windows 下使用 socket 前，必须先调用 WSAStartup 初始化 Winsock 环境。
		if (!bInitSockEnv())
		{
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_vecBuffer.resize(BUFFER_SIZE);
		memset(m_vecBuffer.data(), 0, BUFFER_SIZE);

		m_hPktThreadReadyEvt = CreateEvent(NULL, TRUE, FALSE, NULL);

		struct 
		{
			UINT message;
			PKTFUNC func;
		}funcs[] = {
			{WM_SEND_PACK, &CClientSocket::sendPack},
			{0, NULL}
		};
		for (int i = 0; funcs[i].message != 0; i++)
		{
			if (!m_mapPktFunc.insert(
				std::pair<UINT, PKTFUNC>(funcs[i].message, funcs[i].func)).second)
			{
				TRACE("插入失败！消息值: %d 函数值: %08X 序号: %d\r\n", funcs[i].message, funcs[i].func, i);
			}
		}
		m_mapPktFunc;
	}

CClientSocket::CClientSocket(const CClientSocket& ss)
{
		m_bAutoClosed = ss.m_bAutoClosed;
		m_Sock = ss.m_Sock;
		m_nIP = ss.m_nIP;
		m_nPort = ss.m_nPort;
		for (auto it = ss.m_mapPktFunc.begin(); it != ss.m_mapPktFunc.end(); it++)
		{
			m_mapPktFunc.insert(std::pair<UINT, PKTFUNC>(it->first, it->second));
		}

	}

#pragma warning(push)
#pragma warning(disable: 4716)
CClientSocket& CClientSocket::operator=(const CClientSocket& ss)
{}
#pragma warning(pop)

CClientSocket::~CClientSocket()
{
		// 关闭监听 socket，并释放 Winsock 环境。
		closesocket(m_Sock);
		CloseHandle(m_hPktThreadReadyEvt);
		WSACleanup();
	}

BOOL CClientSocket::bInitSockEnv()
{
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)//TODO：返回值处理
		{
			return FALSE;
		}
		return TRUE;
	}

void CClientSocket::releaseInstance()
{
		if (m_Instance != NULL)
		{
			CClientSocket* tmp = m_Instance;
			m_Instance = NULL;
			delete tmp;
		}
	}

bool CClientSocket::bSend(const char* pData, int nSize)
{
		if (m_Sock == -1)
		{
			return false;
		}
		return send(m_Sock, pData, nSize, 0) > 0;
	}

bool CClientSocket::bSend(const CPacket& pack)
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

PacketData::PacketData(const char* pData, size_t nLen, UINT mode, LPARAM nParam)
{
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);
		nMode = mode;
		lParam = nParam;
	}

PacketData::PacketData(const PacketData& data)
{
		strData = data.strData;
		nMode = data.nMode;
		lParam = data.lParam;
	}

PacketData& PacketData::operator=(const PacketData& data)
{
		if (this != &data)
		{
			strData = data.strData;
			nMode = data.nMode;
			lParam = data.lParam;
		}

		return *this;
	}

file_info::file_info()
{
		bIsInvalid = FALSE;
		bIsDirectory = -1;
		bHasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}

MouseEvent::MouseEvent()
{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}

CPacket::CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0)
{}

CPacket::CPacket(const CPacket& packet)
{
		sHead = packet.sHead;
		nLength = packet.nLength;
		sCmd = packet.sCmd;
		strData = packet.strData;
		sSum = packet.sSum;
	}

CPacket& CPacket::operator=(const CPacket& packet)
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

CPacket::CPacket(const BYTE* pData, size_t& nSize)
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

CPacket::CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
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

int CPacket::Size()
{
		return nLength + 6;
	}

const char* CPacket::Data(std::string& strOut) const
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

CPacket::~CPacket()
{
	}

std::string GetSockErrInfo(int wsaErrcode)
{
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);

	return ret;
}

bool CClientSocket::bSendPkt(HWND hWnd, const CPacket& reqPkt, bool bIsAutoClosed, LPARAM lParam)
{
	if (m_hPktThread == INVALID_HANDLE_VALUE)
	{
		ResetEvent(m_hPktThreadReadyEvt);

		m_hPktThread = (HANDLE)_beginthreadex(
			NULL,
			0,
			&CClientSocket::threadPktHandleEntry,
			this,
			0,
			&m_hPktThreadID
		);

		if (WaitForSingleObject(m_hPktThreadReadyEvt, 3000) != WAIT_OBJECT_0)
		{
			return false;
		}
	}
	UINT nMode = bIsAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strReqOut;
	reqPkt.Data(strReqOut);
	PACKETDATA* pPktData = new PACKETDATA(strReqOut.c_str(), strReqOut.size(), nMode, lParam);
	bool bRet = PostThreadMessage(
		m_hPktThreadID, 
		WM_SEND_PACK, 
		(WPARAM)pPktData,
		(LPARAM)hWnd
	);
	if (!bRet)
	{
		delete pPktData;
	}

	return bRet;
}

unsigned __stdcall CClientSocket::threadPktHandleEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;

	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	SetEvent(thiz->m_hPktThreadReadyEvt);

	thiz->threadPktHandle2();

	_endthreadex(0);

	return 0;
}

void CClientSocket::threadPktHandle2()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		
		if (m_mapPktFunc.find(msg.message) != m_mapPktFunc.end())
		{
			(this->*m_mapPktFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
			
		}
	}
}

void CClientSocket::sendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	PACKETDATA reqPktData = *(PACKETDATA*)wParam;
	delete (PACKETDATA*)wParam;

	HWND hWnd = (HWND)lParam;

	if (bInitSocket())
	{
		int ret = send(m_Sock, (char*)reqPktData.strData.c_str(), (int)reqPktData.strData.size(), 0);
		if (ret > 0)
		{
			int nIndex = 0; 
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);
			char* pAckBuffer = (char*)strBuffer.c_str();
			while (m_Sock != INVALID_SOCKET)
			{
				int nRecvLen = recv(m_Sock, pAckBuffer + nIndex, BUFFER_SIZE - nIndex, 0);
				if (nRecvLen > 0 || nIndex > 0)
				{
					//更新数据在buffer中的存储索引值index：将recv的数据长度len加到上一次的index
					nIndex += nRecvLen;
					//将buffer存储的数据长度改为当前buffer存储数据的索引位置
					size_t nSize = (size_t)nIndex;
					//按引用传入当前数据的长度len，将buffer解析，将数据封装为Packet并返回封装了的数据的长度len
					CPacket ackPkt((BYTE*)pAckBuffer, nSize);
					if (nSize > 0)
					{
						::SendMessage(hWnd, WM_SEND_ACK, (WPARAM)new CPacket(ackPkt), reqPktData.lParam);
						//将解析到的数据从buffer中移走
						memmove(pAckBuffer, pAckBuffer + nSize, nIndex - nSize);
						//变更数据在buffer中的存储索引值index，减掉解析到的数据长度len
						nIndex -= (int)nSize;

						if(reqPktData.nMode & CSM_AUTOCLOSE)
						{
							CloseSocket();
							return;
						}
					}
				}
				else
				{
					CloseSocket();
					::SendMessage(hWnd, WM_SEND_ACK, NULL, 1);
					TRACE("发送应答包结束！！");
				}
			}
		}
		else
		{
			CloseSocket();
			::SendMessage(hWnd, WM_SEND_ACK, NULL, -1);
			TRACE("发送请求包失败！！");
		}
	}
	else
	{
		::SendMessage(hWnd, WM_SEND_ACK, NULL, -2);
		TRACE("网络初始化失败！！");

	}
}
