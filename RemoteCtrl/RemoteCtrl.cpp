// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServSocket.h"



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

//查看磁盘分区
#include <direct.h>
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

//查看指定目录下的文件
#include <io.h>
#include <list>
typedef struct file_info
{
    file_info()
    {
        bIsInvalid = FALSE;
        bIsDirectory = -1;
        bHasNext = TRUE;
        memset(szFileName, 0, sizeof(szFileName));
    }
    BOOL bIsInvalid;            //是否无效：0否 1是
    BOOL bIsDirectory;          //是否为目录：0否 1是
    BOOL bHasNext;              //是否还有下一个文件：0无 1有
    char szFileName[256];       //文件名
}FILEINFO, * pFILEINFO;

int MakeDirecoryInfo()
{
    std::string strPath;
    //std::list<FILEINFO> lstFileInfos;

    if (!(CServSocket::getInstance()->bGetFilePath(strPath)))
    {
        OutputDebugString(_T("当前的命令不是获取文件列表，命令解析错误！"));
        return -1;
    }

    if (_chdir(strPath.c_str()) != 0)
    {
        FILEINFO fInfo;
        fInfo.bIsInvalid    = TRUE;
        fInfo.bIsDirectory  = TRUE;
        fInfo.bHasNext      = FALSE;
        memcpy(fInfo.szFileName, strPath.c_str(), strPath.size());
        //lstFileInfos.push_back(fInfo);
        CPacket pack(2, (BYTE*)&fInfo, sizeof(fInfo));
        CServSocket::getInstance()->bSend(pack);

        OutputDebugString(_T("无权限访问目录！"));
        return -2;
    }

    _finddata_t fData;
    intptr_t hFind = 0;
    if ((hFind = _findfirst("*", &fData)) == -1)
    {
        OutputDebugString(_T("未找到任何文件！"));
        return -3;
    }
    do {
        FILEINFO fInfo;
        fInfo.bIsDirectory = ((fData.attrib & _A_SUBDIR) != 0);
                            //(fData.attrib & _A_SUBDIR) != 0 ==>TRUE
        memcpy(fInfo.szFileName, fData.name, strlen(fData.name));
        //lstFileInfos.push_back(fInfo);
        CPacket pack(2, (BYTE*)&fInfo, sizeof(fInfo));
        CServSocket::getInstance()->bSend(pack);//获取一个文件就发送一个
    } while (!_findnext(hFind, &fData));
    
    FILEINFO fInfo;
    fInfo.bHasNext = FALSE;//告诉控制端没有下一个文件了，不必继续等待
    CPacket pack(2, (BYTE*)&fInfo, sizeof(fInfo));
    CServSocket::getInstance()->bSend(pack);

    return 0;
}

//运行文件
int RunFile()
{
    std::string strPath;

    CServSocket::getInstance()->bGetFilePath(strPath);
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);

    CPacket pack(3, NULL, 0);
    CServSocket::getInstance()->bSend(pack);

    return 0;
}

//下载文件
int DownLoadFile()
{
    std::string strPath;
    long long data = 0;

    CServSocket::getInstance()->bGetFilePath(strPath);
    FILE* pFile = NULL;
    errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
    if (err != 0)
    {
        CPacket pack(4, (BYTE*)&data, 8);
        CServSocket::getInstance()->bSend(pack);
        return -1;
    }

    if(pFile != NULL)
    {
        fseek(pFile, 0, SEEK_END);
        data = _ftelli64(pFile);
        CPacket head(4, (BYTE*)&data, 8);
        fseek(pFile, 0, SEEK_SET);
        CServSocket::getInstance()->bSend(head);

        char buffer[1024] = "";
        size_t rlen = 0;
        do {
            rlen = fread(buffer, 1, 1024, pFile);
            CPacket pack(4, (BYTE*)buffer, rlen);
            CServSocket::getInstance()->bSend(pack);
        } while (rlen >= 1024);

        fclose(pFile);
    }
    CPacket pack(4, NULL, 0);
    CServSocket::getInstance()->bSend(pack);

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
            {//需求：处理文件
            case 1:// ==> 需要查看磁盘分区
                MakeDriverInfo();
                break;
            case 2:// ==> 需要查看指定目录下的文件
                MakeDirecoryInfo();
                break;
            case 3:// ==> 需要打开文件
                RunFile();
                break;
            case 4:
                DownLoadFile();
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

