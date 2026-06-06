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

void CClientSocket::threadPktHandleEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadPktHandle();

	_endthread();
}

void CClientSocket::threadPktHandle()
{
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int nIndex = 0;

	while (m_Sock != INVALID_SOCKET)
	{
		if (m_lstSendPkt.size() > 0)
		{
			TRACE("lst Send Size = %d\r\n", m_lstSendPkt.size());
			CPacket& head = m_lstSendPkt.front();
			if (!bSend(head))
			{
				TRACE("发送失败！！\r\n");
				continue;
			}

			auto pr = m_mapAck.insert(std::pair<HANDLE, 
				std::list<CPacket>>(head.hEvent, std::list<CPacket>()));

			int nRecvLen = recv(m_Sock, pBuffer + nIndex, BUFFER_SIZE - nIndex, 0);
			if (nRecvLen > 0 || nIndex > 0)
			{
				nIndex += nRecvLen;
				size_t nSize = (size_t)nIndex;
				CPacket pack((BYTE*)pBuffer, nSize);
				if (nSize > 0)
				{
					//TODO:通知对应事件
					pack.hEvent = head.hEvent;
					pr.first->second.push_back(pack);
					SetEvent(head.hEvent);
				}
			}
			else if (nRecvLen <= 0 && nIndex <= 0)
			{
				CloseSocket();
			}

			m_lstSendPkt.pop_front();
		}
	}
	CloseSocket();
}
