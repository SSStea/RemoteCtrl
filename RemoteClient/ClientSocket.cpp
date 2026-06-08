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

bool CClientSocket::bSendPkt(const CPacket& reqPkt, std::list<CPacket>& out_lstAckPkts, 
	bool bIsAutoClosed)
{
	if (m_Sock == INVALID_SOCKET)
	{
		/*if (!bInitSocket())
		{
			return false;
		}*/
		_beginthread(&CClientSocket::threadPktHandleEntry, 0, this);
	}

	m_mapAck.insert(std::pair<HANDLE,
		std::list<CPacket>&>(reqPkt.hEvent, out_lstAckPkts));
	m_mapAutoClsoed.insert(std::pair<HANDLE, bool>(reqPkt.hEvent, bIsAutoClosed));
	TRACE("cmd %d event %08X thread id %d\r\n", reqPkt.sCmd, reqPkt.hEvent, GetCurrentThreadId());
	m_lstSendPkt.push_back(reqPkt);

	WaitForSingleObject(reqPkt.hEvent, INFINITE);

	auto it = m_mapAck.find(reqPkt.hEvent);
	if (it != m_mapAck.end())
	{
		m_mapAck.erase(it);
		return true;
	}
	return false;
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

	bInitSocket();

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

			auto itAckPtks = m_mapAck.find(head.hEvent);
			auto itAutoClose = m_mapAutoClsoed.find(head.hEvent);
			if (itAckPtks != m_mapAck.end() && itAutoClose != m_mapAutoClsoed.end())
			{
				do
				{
					int nRecvLen = recv(m_Sock, pBuffer + nIndex, BUFFER_SIZE - nIndex, 0);
					if (nRecvLen > 0 || nIndex > 0)//表示读到或者缓冲区里有数据
					{
						//更新数据在buffer中的存储索引值index：将recv的数据长度len加到上一次的index
						nIndex += nRecvLen;
						//将buffer存储的数据长度改为当前buffer存储数据的索引位置
						size_t nSize = (size_t)nIndex;
						//按引用传入当前数据的长度len，将buffer解析，将数据封装为Packet并返回封装了的数据的长度len
						CPacket pack((BYTE*)pBuffer, nSize);
						if (nSize > 0)
						{
							//TODO:通知对应事件
							pack.hEvent = head.hEvent;
							itAckPtks->second.push_back(pack);
							//将解析到的数据从buffer中移走
							memmove(pBuffer, pBuffer + nSize, nIndex - nSize);
							//变更数据在buffer中的存储索引值index，减掉解析到的数据长度len
							nIndex -= nSize;
							if (itAutoClose->second)
							{
								SetEvent(head.hEvent);
								break;
							}
						}
					}
					else if (nRecvLen <= 0 && nIndex <= 0)
					{
						CloseSocket();
						//等待服务器关闭之后再通知这个命令的接收包事件完成
						SetEvent(head.hEvent);
						break;
					}
				} while (!itAutoClose->second);
			}

			m_mapAutoClsoed.erase(itAutoClose);
			m_lstSendPkt.pop_front();
			if (!bInitSocket())
			{
				bInitSocket();
			}
		}
	}
	CloseSocket();
}
