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
		m_hPktThread = (HANDLE)_beginthreadex(
			NULL,
			0,
			&CClientSocket::threadPktHandleEntry,
			this,
			0,
			&m_hPktThreadID
		);
	}
	UINT nMode = bIsAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strReqOut;
	reqPkt.Data(strReqOut);
	bool bRet = PostThreadMessage(
		m_hPktThreadID, 
		WM_SEND_PACK, 
		(WPARAM)new PACKETDATA(strReqOut.c_str(), strReqOut.size(), nMode, lParam),
		(LPARAM)hWnd
	);

	return bRet;
}

unsigned CClientSocket::threadPktHandleEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
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
