// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServSocket.h"
#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

void Dump(BYTE* pData, size_t nSize)
{
    std::string strOut;
    for (size_t i = 0; i < nSize; i++)
    {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0))
        {
            strOut += "\n";
        }
        snprintf(buf, sizeof(buf), "%02X", pData[i] & 0xFF);
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());
}

int MakeDriverInfo()//1==>A 2==>B 3==>C ... 26==>Z
{
    std::string strRes;
    for (int i = 1; i <= 26; i++)
    {
        if (_chdrive(i) == 0)//代表能切换到这个盘
        {
            if (strRes.size() > 0)
            {
                strRes += ',';
            }
            strRes += ('A' + i - 1);
        }
    }
    CPacket pack(1, (BYTE*)strRes.c_str(), strRes.size());//重载了一个打包用的构造函数
    Dump((BYTE*)pack.Data(), pack.Size());

    //CServSocket::getInstance()->bSend(pack);
    return 0;
}

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
            // 取得服务端 socket 单例。
            // getInstance 会确保 Winsock 环境已经初始化，并且服务端 socket 对象只创建一次。
            //CServSocket*    pServer = CServSocket::getInstance();
            //// 统计 accept 客户端失败的次数，连续失败太多就结束程序。
            //int             nCount = 0;
            //// 初始化监听 socket：创建地址、绑定 9527 端口、进入监听状态。
            //if (!pServer->bInitSocket())
            //{
            //    MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态"),
            //        _T("网络初始化失败"), MB_OK | MB_ICONERROR);
            //    exit(0);
            //}
            //// 服务端主循环。
            //// 只要单例对象还存在，就不断等待客户端接入并处理客户端命令。
            //while (CServSocket::getInstance() != NULL)
            //{
            //    // 等待客户端连接。
            //    // 如果没有客户端连接，bAcceptClient 内部的 accept 会阻塞等待。
            //    if (!pServer->bAcceptClient())
            //    {
            //        if (nCount >= 3)
            //        {
            //            MessageBox(NULL, _T("多次无法正常接入用户，结束程序"),
            //                _T("接入用户失败！"), MB_OK | MB_ICONERROR);
            //            exit(0);
            //        }
            //        MessageBox(NULL, _T("无法正常接入用户，自动重试"), 
            //            _T("接入用户失败！"), MB_OK | MB_ICONERROR);
            //        nCount++;
            //    }
            //    // 客户端连接成功后，进入命令处理逻辑。
            //    // 当前 dealCommand 里还没有真正解析命令，只是在循环 recv。
            //    int nRet = pServer->dealCommand();
            //}

            int nCmd = 1;
            switch (nCmd)
            {
            case 1:
                //需求：处理文件 ==> 需要查看磁盘分区
                MakeDriverInfo();
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

