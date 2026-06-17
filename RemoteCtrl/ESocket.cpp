#include "pch.h"
#include "ESocket.h"

ESocket::ESocket(ETYPE nType, int nProtocol)
{
	m_socket = socket(PF_INET, (int)nType, nProtocol);
	m_type = nType;
	m_protocol = nProtocol;
}

ESocket::ESocket(const ESocket& sock)
{
	m_socket = socket(PF_INET, (int)sock.m_type, sock.m_protocol);
	m_type = sock.m_type;
	m_protocol = sock.m_protocol;
	m_addr = sock.m_addr;
}

ESocket& ESocket::operator=(const ESocket& sock)
{
	if (*this != sock)
	{
		m_socket = socket(PF_INET, (int)sock.m_type, sock.m_protocol);
		m_type = sock.m_type;
		m_protocol = sock.m_protocol;
		m_addr = sock.m_addr;
	}
	return *this;
}

ESocket::~ESocket()
{
	close();
}

ESocket::operator SOCKET()
{
	return m_socket;
}

ESocket::operator SOCKET() const
{
	return m_socket;
}

bool ESocket::operator==(SOCKET sock) const
{
	return m_socket == sock;
}

int ESocket::listen(int backlog)
{
	if (m_type != ETYPE::ETyepTcp)
		return -1;

	return ::listen(m_socket, backlog);
}

int ESocket::bind(const std::string& ip, short port)
{
	m_addr = ESockAddrIn(ip, port);
	return ::bind(m_socket, m_addr, m_addr.nSize());
}

int ESocket::send(const EBuffer& buffer)
{
	return ::send(m_socket, buffer, (int)buffer.size(), 0);
}

int ESocket::recv(EBuffer& buffer)
{
	return ::recv(m_socket, buffer, (int)buffer.size(), 0);
}

int ESocket::sendto(const EBuffer& buffer, const ESockAddrIn& to)
{
	return ::sendto(m_socket, buffer, (int)buffer.size(), 0, to, to.nSize());
}

int ESocket::recvfrom(EBuffer& buffer, ESockAddrIn& from)
{
	int len = from.nSize();
	int ret = ::recvfrom(m_socket, buffer, (int)buffer.size(), 0, from, &len);
	if (ret > 0)
	{
		from.update();
	}

	return ret;
}

void ESocket::close()
{
	if (m_socket != INVALID_SOCKET)
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}
}

ESockAddrIn::ESockAddrIn()
{
	memset(&m_addr, 0, sizeof(m_addr));
	m_port = -1;
}

ESockAddrIn::ESockAddrIn(sockaddr_in addr)
{
	memcpy(&m_addr, &addr, sizeof(addr));
	m_ip = inet_ntoa(m_addr.sin_addr);
	m_port = ntohs(m_addr.sin_port);
}

ESockAddrIn::ESockAddrIn(UINT nIP, short nPort)
{
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(nPort);
	m_addr.sin_addr.s_addr = htonl(nIP);
	m_ip = inet_ntoa(m_addr.sin_addr);
	m_port = nPort;
}

ESockAddrIn::ESockAddrIn(const std::string& strIP, short nPort)
{
	m_ip = strIP;
	m_port = nPort;
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(nPort);
	m_addr.sin_addr.s_addr = inet_addr(strIP.c_str());
}

ESockAddrIn::ESockAddrIn(const ESockAddrIn& addr)
{
	memcpy(&m_addr, &addr.m_addr, sizeof(m_addr));
	m_ip = addr.m_ip;
	m_port = addr.m_port;
}

ESockAddrIn& ESockAddrIn::operator=(const ESockAddrIn& addr)
{
	if (this != &addr)
	{
		memcpy(&m_addr, &addr.m_addr, sizeof(m_addr));
		m_ip = addr.m_ip;
		m_port = addr.m_port;
	}

	return *this;
}

ESockAddrIn::operator sockaddr* () const
{
	return (sockaddr*)&m_addr;
}

ESockAddrIn::operator void* () const
{
	return (void*)&m_addr;
}

std::string ESockAddrIn::strGetIP() const
{
	return m_ip;
}

short ESockAddrIn::nGetPort() const
{
	return m_port;
}

void ESockAddrIn::update()
{
	m_ip = inet_ntoa(m_addr.sin_addr);
	m_port = ntohs(m_addr.sin_port);
}

int ESockAddrIn::nSize() const
{
	return sizeof(sockaddr_in);
}

EBuffer::operator char* () const
{
	return (char*)c_str();
}

EBuffer::operator const char* () const
{
	return c_str();
}

EBuffer::operator BYTE* () const
{
	return (BYTE*)c_str();
}

EBuffer::operator void* () const
{
	return (void*)c_str();
}

void EBuffer::UpDate(void* buffer, size_t size)
{
	resize(size);
	memcpy((void*)c_str(), buffer, size);
}
