// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "ServSocket.h"
#include "Command.h"
#include <conio.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//系统变量路径
#define SYSTEMPATH TEXT("C:\\Windows\\System32\\RemoteCtrl.exe")

// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

//开机启动的时候，程序的权限是跟随启动用户的
//如果两者权限不一致，则会导致程序启动失败
//开机启动对环境变量有影响，如果依赖dll库则可能启动失败
//【可以通过将dll库复制到system32或sysWOW64下面】
bool ChooseAutoInvoke()
{
    CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
    CString strInfo = _T("该程序只允许用于合法的用途！\n");
    strInfo += _T("继续运行该程序，将使得这台机器处于被监控状态\n");
    strInfo += _T("如果你不希望这样，请点击“取消”按钮，退出程序\n");
    strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行\n");
    strInfo += _T("按下“否”按钮，程序只运行一次，不会在系统内留下任何东西\n");

    int nRet = MessageBox(NULL, strInfo, _T("WARNING"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
	if (nRet == IDYES)
	{
		if (!CEdoyunTool::WriteRefisterTable(SYSTEMPATH)) {}//尝试方式二设置开机自启 写入注册表自启动 
		else
		{
			MessageBox(NULL, TEXT("设置开机自启动失败\r\n"), TEXT("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
	}
	else if (nRet == IDCANCEL) return false;
	return true;
}

void udp_server();

void udp_client(bool bIsHost = true);

void initsock()
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
}

void clearsock()
{
	WSACleanup();
}

int main(int argc, char* argv[])
{
	//if (!CEdoyunTool::IsAdmin())//管理员用户  TODO:这里条件取反 为了测试方便避免提权操作(也就是普通用户会进入此逻辑)
	//{
	if (!CEdoyunTool::Init()) return 1;//MFC命令行项目初始化
	//MessageBox(NULL, TEXT("管理员"), TEXT("用户状态"), 0);
	//if (ChooseAutoInvoke())//设置开机自启
	//{
 //       // 创建命令处理模块
 //       CCommand cmd;
 //       ChooseAutoInvoke();

 //       // 取得服务端 socket 单例。
 //       // getInstance 会确保 Winsock 环境已经初始化，并且服务端 socket 对象只创建一次。
 //       CServSocket*    pServer = CServSocket::getInstance();

 //       // 网络单例socket的一系列操作、处理接收到的命令
 //       int nRet = pServer->nRun(&CCommand::RunCommand, &cmd);
 //       switch (nRet)
 //       {
 //       case -1:
	//		{
	//			MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态"),
	//				_T("网络初始化失败"), MB_OK | MB_ICONERROR);
	//			exit(0);
	//		}
 //           break;
 //       case -2:
 //           {
	//			MessageBox(NULL, _T("多次无法正常接入用户，结束程序"),
	//				_T("接入用户失败！"), MB_OK | MB_ICONERROR);
	//			exit(0);
 //           }
 //           break;
 //       default:
 //           break;
 //       }     
	//}

	initsock();
	if (argc == 1)
	{
		char wstrDir[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, wstrDir);
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		memset(&si, 0, sizeof(si));
		memset(&pi, 0, sizeof(pi));
		std::string strCmd = argv[0];
		strCmd += " 1";
		BOOL bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
		if (bRet)
		{
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			TRACE("进程ID：%d\r\n", pi.dwProcessId);
			TRACE("线程ID：%d\r\n", pi.dwThreadId);

			strCmd += " 2";
			bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
			if (bRet)
			{
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
				TRACE("进程ID：%d\r\n", pi.dwProcessId);
				TRACE("线程ID：%d\r\n", pi.dwThreadId);

				udp_server();//服务器
			}
		}
	}
	else if (argc == 2)
	{//主客户端
		udp_client();
	}
	else
	{
		udp_client(false);
	}
	clearsock();
	//}
	//else//普通用户   需要提权创建新进程
	//{
	//	if (CEdoyunTool::RunAsAdmin() == false) return 1;
	//	MessageBox(NULL, TEXT("普通用户"), TEXT("用户状态"), 0);
	//}
	return 0;
}


void udp_server()
{
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	
	SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)
	{
		printf("%s(%d):%s server ERROR(%d)!!!\r\n", __FILE__, __LINE__, 
			__FUNCTION__, WSAGetLastError());
		return;
	}

	std::list<sockaddr_in> lstClients;

	sockaddr_in server, client;
	int len = sizeof(sockaddr_in);
	memset(&server, 0, sizeof(sockaddr_in));
	memset(&client, 0, sizeof(sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(20000);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (bind(sock, (const sockaddr*)&server, sizeof(sockaddr)) == -1)
	{
		printf("%s(%d):%s server ERROR(%d)!!!\r\n", __FILE__, __LINE__,
			__FUNCTION__, WSAGetLastError());
		closesocket(sock);
		return;
	}

	char buffer[4096] = "";
	int ret = 0;
	while (!_kbhit())
	{
		ret = recvfrom(sock, buffer, sizeof(buffer), 0, (sockaddr*)&client, &len);
		if (ret > 0)
		{
			if (lstClients.size() <= 0)
			{
				lstClients.push_back(client);
				printf("%s(%d):%s server ip %08X port %d\r\n", __FILE__, __LINE__,
					__FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));
				ret = sendto(sock, buffer, ret, 0, (const sockaddr*)&client, len);
				printf("%s(%d):%s server \r\n", __FILE__, __LINE__, __FUNCTION__);
			}
			//CEdoyunTool::Dump((BYTE*)buffer, ret);
			else
			{
				memcpy((void*)buffer, &lstClients.front(), sizeof(lstClients.front()));
				ret = sendto(sock, buffer, sizeof(lstClients.front()), 0, (const sockaddr*)&client, len);
				printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
			}
		}
		else
		{
			printf("%s(%d):%s server ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, 
				__FUNCTION__, WSAGetLastError(), ret);
		}
	}
	closesocket(sock);
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);

	getchar();
}

void udp_client(bool bIsHost)
{
	Sleep(2000);
	SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)
	{
		printf("%s(%d):%s ERROR(%d)!!!\r\n", __FILE__, __LINE__,
			__FUNCTION__, WSAGetLastError());
		return;
	}

	sockaddr_in server, client;
	int len = sizeof(sockaddr_in);
	memset(&server, 0, sizeof(sockaddr_in));
	memset(&client, 0, sizeof(sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(20000);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");


	if (bIsHost)
	{//主客户端
		printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);

		std::string strMsg = "hello world!\n";
		int ret = sendto(sock, strMsg.c_str(), (int)strMsg.length(), 0,
			(const sockaddr*)&server, sizeof(server));
		printf("%s(%d):%s host ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, 
			__FUNCTION__, WSAGetLastError(), ret);

		if (ret > 0)
		{
			strMsg.resize(1024);
			memset((char*)strMsg.c_str(), 0, strMsg.size());

			ret = recvfrom(sock, (char*)strMsg.c_str(), (int)strMsg.length(), 0,
				(sockaddr*)&client, &len);
			printf("%s(%d):%s host ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__,
				__FUNCTION__, WSAGetLastError(), ret);

			if (ret > 0)
			{
				printf("%s(%d):%s host ip %08X port %d\r\n", __FILE__, __LINE__,
					__FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));
				printf("%s(%d):%s host ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, strMsg.size());
			}

			ret = recvfrom(sock, (char*)strMsg.c_str(), (int)strMsg.length(), 0,
				(sockaddr*)&client, &len);
			printf("%s(%d):%s host ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__,
				__FUNCTION__, WSAGetLastError(), ret);

			if (ret > 0)
			{
				printf("%s(%d):%s host ip %08X port %d\r\n", __FILE__, __LINE__,
					__FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));
				printf("%s(%d):%s host ret = %s\r\n", __FILE__, __LINE__, __FUNCTION__, strMsg.c_str());
			}
		}
	}
	else
	{//从客户端
		printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);

		std::string strMsg = "hello world!\n";
		int ret = sendto(sock, strMsg.c_str(), (int)strMsg.length(), 0,
			(const sockaddr*)&server, sizeof(server));
		printf("%s(%d):%s client ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, 
			__FUNCTION__, WSAGetLastError(), ret);

		if (ret > 0)
		{
			strMsg.resize(1024);
			memset((char*)strMsg.c_str(), 0, strMsg.size());

			ret = recvfrom(sock, (char*)strMsg.c_str(), (int)strMsg.length(), 0,
				(sockaddr*)&client, &len);
			printf("%s(%d):%s client ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__,
				__FUNCTION__, WSAGetLastError(), ret);
			
			if (ret > 0)
			{
				sockaddr_in addr;
				memcpy(&addr, strMsg.c_str(), sizeof(addr));
				sockaddr_in* pAddr = (sockaddr_in*)&addr;
				printf("%s(%d):%s client ip %08X port %d\r\n", __FILE__, __LINE__,
					__FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));
				printf("%s(%d):%s client ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, strMsg.size());
				
				printf("%s(%d):%s client ip %08X port %d\r\n", __FILE__, __LINE__,
					__FUNCTION__, pAddr->sin_addr.s_addr, ntohs(pAddr->sin_port));

				strMsg = "hello i am client!\r\n";
				ret = sendto(sock, (char*)strMsg.c_str(), (int)strMsg.length(), 0, 
					(const sockaddr*)pAddr, sizeof(sockaddr_in));
				printf("%s(%d):%s client ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__,
					__FUNCTION__, WSAGetLastError(), ret);
			}
		}
	}
	closesocket(sock);
}