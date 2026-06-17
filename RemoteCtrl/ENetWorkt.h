#pragma once
#include "ESocket.h"
#include "EdoyunThread.h"
#include <conio.h>
#include <list>

class CENetWorkt
{

};

typedef int(*AcceptFunc)(void* arg, ESOCKET& client);
typedef int(*RecvFunc)(void* arg, const EBuffer& buffer);
typedef int(*SendFunc)(void* arg, const ESOCKET& client, int ret);
typedef int(*RecvFromFunc)(void* arg, const EBuffer& buffer, ESockAddrIn& addr);
typedef int(*SendToFunc)(void* arg, const ESockAddrIn& addr, int ret);

class EServerParameter
{
public:
	EServerParameter(
		const std::string& ip = "0.0.0.0",
		short port = 9527, ETYPE type = ETYPE::ETyepTcp, 
		AcceptFunc afunc = NULL, 
		RecvFunc rfunc = NULL, 
		SendFunc sfunc = NULL,
		RecvFromFunc rffunc = NULL,
		SendToFunc	stfunc = NULL
	);

	//输入
	EServerParameter& operator<<(AcceptFunc func);
	EServerParameter& operator<<(RecvFunc func);
	EServerParameter& operator<<(SendFunc func);
	EServerParameter& operator<<(RecvFromFunc func);
	EServerParameter& operator<<(SendToFunc func);
	EServerParameter& operator<<(const std::string& ip);
	EServerParameter& operator<<(short port);
	EServerParameter& operator<<(ETYPE type);

	//输出
	EServerParameter& operator>>(AcceptFunc& func);
	EServerParameter& operator>>(RecvFunc& func);
	EServerParameter& operator>>(SendFunc& func);
	EServerParameter& operator>>(RecvFromFunc& func);
	EServerParameter& operator>>(SendToFunc& func);
	EServerParameter& operator>>(std::string& ip);
	EServerParameter& operator>>(short& port);
	EServerParameter& operator>>(ETYPE& type);

	//复制构造函数，等于号重载，用于同类型赋值
	EServerParameter(const EServerParameter& param);
	EServerParameter& operator=(const EServerParameter& param);

	std::string m_ip;
	short m_port;
	ETYPE m_type;
	AcceptFunc m_accept;
	RecvFunc m_recv;
	SendFunc m_send;
	RecvFromFunc m_recvfrom;
	SendToFunc m_sendto;

};

class EServer : public ThreadFuncBase
{
public:
	EServer(const EServerParameter& param);
	~EServer();

	int Invoke(void* arg);

	int Send(ESOCKET& client, const EBuffer& buffer);

	int SendTo(ESockAddrIn& addr, const EBuffer& buffer);

	int Stop();

private:
	int threadWorkFunc();
	int threadUDPWorkFunc();
	int threadTCPWorkFunc();

private:
	EServerParameter m_params;
	void* m_arg;
	EdoyunThread m_thread;
	ESOCKET m_sock;
	std::atomic<bool> m_stop;
};