#include "pch.h"
#include "ENetWorkt.h"
#include <memory>

EServer::EServer(const EServerParameter& param) : m_stop(false), m_arg(NULL)
{
	m_params = param;
	m_thread.UpdateWorker(ThreadWorker(this, (FUNCTYPE)&EServer::threadWorkFunc));
}

EServer::~EServer()
{
	Stop();
}

int EServer::Invoke(void* arg)
{
	m_sock.reset(new ESocket(m_params.m_type));
	if (*m_sock == INVALID_SOCKET)
	{
		printf("%s(%d):%s server ERROR(%d)!!!\r\n", __FILE__, __LINE__,
			__FUNCTION__, WSAGetLastError());
		return -1;
	}

	std::list<ESockAddrIn> lstClients;

	if (m_params.m_type == ETYPE::ETyepTcp)
	{
		if (m_sock->listen() == -1)
			return -1;
	}

	if (m_sock->bind(m_params.m_ip, m_params.m_port) == -1)
	{
		printf("%s(%d):%s server ERROR(%d)!!!\r\n", __FILE__, __LINE__,
			__FUNCTION__, WSAGetLastError());
		return -3;
	}

	if (!m_thread.Start())
		return -4;

	m_arg = arg;

	return 0;
}

int EServer::Send(ESOCKET& client, const EBuffer& buffer)
{
	int ret = m_sock->send(buffer);
	if (m_params.m_send)
	{
		m_params.m_send(m_arg, client, ret);
	}
	return ret;
}

int EServer::SendTo(ESockAddrIn& addr, const EBuffer& buffer)
{
	int ret = m_sock->sendto(buffer, addr);
	if (m_params.m_sendto)
	{
		m_params.m_sendto(m_arg, addr, ret);
	}
	return ret;
}

int EServer::Stop()
{
	if (!m_stop)
	{
		m_sock->close();
		m_stop = true;
		m_thread.Stop();
	}

	return 0;
}

int EServer::threadWorkFunc()
{
	if (m_params.m_type == ETYPE::ETyepTcp)
	{
		return threadTCPWorkFunc();
	}
	else
	{
		return threadUDPWorkFunc();
	}
}

int EServer::threadUDPWorkFunc()
{
	EBuffer buffer(1024 * 2256);
	ESockAddrIn client;
	int ret = 0;
	while (!m_stop)
	{
		ret = m_sock->recvfrom(buffer, client);
		if (ret > 0)
		{
			client.update();
			if (m_params.m_recvfrom != NULL)
			{
				m_params.m_recvfrom(m_arg, buffer, client);
			}
		}
		else
		{
			printf("%s(%d):%s server ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__,
				__FUNCTION__, WSAGetLastError(), ret);
			break;
		}
	}
	if (!m_stop)
	{
		m_stop = true;
	}
	m_sock->close();
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);

	return 0;
}

int EServer::threadTCPWorkFunc()
{
	return 0;
}

EServerParameter::EServerParameter(const std::string& ip, 
	short port, 
	ETYPE type, 
	AcceptFunc afunc, 
	RecvFunc rfunc, 
	SendFunc sfunc,
	RecvFromFunc rffunc, 
	SendToFunc stfunc)
{
	m_ip = ip;
	m_port = port;
	m_type = type;
	m_accept = afunc;
	m_recv = rfunc;
	m_send = sfunc;
	m_recvfrom = rffunc;
	m_sendto = stfunc;
}

EServerParameter& EServerParameter::operator<<(AcceptFunc func)
{
	m_accept = func;
	return *this;
}

EServerParameter& EServerParameter::operator<<(RecvFunc func)
{
	m_recv = func;
	return *this;
}

EServerParameter& EServerParameter::operator<<(SendFunc func)
{
	m_send = func;
	return *this;
}

EServerParameter& EServerParameter::operator<<(RecvFromFunc func)
{
	m_recvfrom = func;
	return *this;
}

EServerParameter& EServerParameter::operator<<(SendToFunc func)
{
	m_sendto = func;
	return *this;
}

EServerParameter& EServerParameter::operator<<(const std::string& ip)
{
	m_ip = ip;
	return *this;
}

EServerParameter& EServerParameter::operator<<(short port)
{
	m_port = port;
	return *this;
}

EServerParameter& EServerParameter::operator<<(ETYPE type)
{
	m_type = type;
	return *this;
}

EServerParameter& EServerParameter::operator>>(AcceptFunc& func)
{
	func = m_accept;
	return *this;
}

EServerParameter& EServerParameter::operator>>(RecvFunc& func)
{
	func = m_recv;
	return *this;
}

EServerParameter& EServerParameter::operator>>(SendFunc& func)
{
	func = m_send;
	return *this;
}

EServerParameter& EServerParameter::operator>>(RecvFromFunc& func)
{
	func = m_recvfrom;
	return *this;
}

EServerParameter& EServerParameter::operator>>(SendToFunc& func)
{
	func = m_sendto;
	return *this;
}

EServerParameter& EServerParameter::operator>>(std::string& ip)
{
	ip = m_ip;
	return *this;
}

EServerParameter& EServerParameter::operator>>(short& port)
{
	port = m_port;
	return *this;
}

EServerParameter& EServerParameter::operator>>(ETYPE& type)
{
	type = m_type;
	return *this;
}

EServerParameter::EServerParameter(const EServerParameter& param)
{
	m_ip = param.m_ip;
	m_port = param.m_port;
	m_type = param.m_type;
	m_accept = param.m_accept;
	m_recv = param.m_recv;
	m_send = param.m_send;
	m_recvfrom = param.m_recvfrom;
	m_sendto = param.m_sendto;
}

EServerParameter& EServerParameter::operator=(const EServerParameter& param)
{
	if (this != &param)
	{
		m_ip = param.m_ip;
		m_port = param.m_port;
		m_type = param.m_type;
		m_accept = param.m_accept;
		m_recv = param.m_recv;
		m_send = param.m_send;
		m_recvfrom = param.m_recvfrom;
		m_sendto = param.m_sendto;
	}

	return *this;
}
