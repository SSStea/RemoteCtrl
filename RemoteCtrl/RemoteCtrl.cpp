// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "ServSocket.h"
#include "Command.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;


int main()
{
    int nRetCode = 0;

    // 获取当前程序模块句柄，MFC 初始化需要用到它。
    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误。
        // 这里虽然是控制台入口 main，但项目仍然使用了 MFC 的部分能力，例如 MessageBox。
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            // 创建命令处理模块
            CCommand cmd;

            // 取得服务端 socket 单例。
            // getInstance 会确保 Winsock 环境已经初始化，并且服务端 socket 对象只创建一次。
            CServSocket*    pServer = CServSocket::getInstance();

            // 网络单例socket的一系列操作、处理接收到的命令
            int nRet = pServer->nRun(&CCommand::RunCommand, &cmd);
            switch (nRet)
            {
            case -1:
			    {
				    MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态"),
					    _T("网络初始化失败"), MB_OK | MB_ICONERROR);
				    exit(0);
			    }
                break;
            case -2:
                {
				    MessageBox(NULL, _T("多次无法正常接入用户，结束程序"),
					    _T("接入用户失败！"), MB_OK | MB_ICONERROR);
				    exit(0);
                }
                break;
            default:
                break;
            }     
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}

