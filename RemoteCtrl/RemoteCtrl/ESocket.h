#pragma once
#include <WinSock2.h>
#include <memory>

enum class ETYPE {
	ETyepTcp = 1,
	ETypeUdp
};

class ESockAddrIn
{
public:
	ESockAddrIn();
	ESockAddrIn(sockaddr_in addr);
	ESockAddrIn(UINT nIP, short nPort);
	ESockAddrIn(const std::string& strIP, short nPort);
	ESockAddrIn(const ESockAddrIn& addr);
	ESockAddrIn& operator=(const ESockAddrIn& addr);


	operator sockaddr* () const;
	operator void* () const;

	std::string strGetIP() const;
	short nGetPort() const;

	void update();

	int nSize() const;


private:
	sockaddr_in m_addr;
	std::string m_ip;
	short m_port;
};

class EBuffer : public std::string
{
public:
	EBuffer(size_t size = 0) : std::string()
	{
		if (size > 0)
		{
			resize(size);
			memset(*this, 0, this->size());
		}
	}
	EBuffer(void* buffer, size_t size) : std::string()
	{
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}
	EBuffer(const char* str)
	{
		resize(strlen(str));
		memcpy((void*)c_str(), str, size());
	}

	~EBuffer(){}
	
	operator char* () const;
	operator const char* () const;
	operator BYTE* () const;
	operator void* () const;

	void UpDate(void* buffer, size_t size);
};

class ESocket
{
public:
	ESocket(ETYPE nType = ETYPE::ETyepTcp, int nProtocol = 0);

	ESocket(const ESocket& sock);

	ESocket& operator=(const ESocket& sock);

	~ESocket();

	operator SOCKET();
	operator SOCKET() const;

	bool operator==(SOCKET sock) const;

	int listen(int backlog = 5);

	int bind(const std::string& ip, short port);

	int accept();
	int connect(const std::string& ip, short port);
	int send(const EBuffer& buffer);
	int recv(EBuffer& buffer);

	int sendto(const EBuffer& buffer, const ESockAddrIn& to);
	int recvfrom(EBuffer& buffer, ESockAddrIn& from);

	void close();

private:
	SOCKET m_socket;
	ETYPE m_type;
	int m_protocol;
	ESockAddrIn m_addr;
};

typedef std::shared_ptr<ESocket> ESOCKET;

