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
                strRes.push_back(',');
            }
            strRes.push_back('A' + i - 1);
        }
    }
    strRes.push_back(',');
    CPacket pack(1, (BYTE*)strRes.c_str(), strRes.size());//重载了一个打包用的构造函数
    Dump((BYTE*)pack.Data(), pack.Size());

    CServSocket::getInstance()->bSend(pack);
    return 0;
}

//查看指定目录下的文件
#include <io.h>
#include <list>

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
        fInfo.bHasNext = FALSE;  
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
        TRACE("%s\r\n", fInfo.szFileName);

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
        CPacket pack(4, (BYTE*)&data, 8);//第一个包的数据是文件长度，如果为0证明打开失败
        CServSocket::getInstance()->bSend(pack);
        return -1;
    }

    if(pFile != NULL)
    {
        fseek(pFile, 0, SEEK_END);
        data = _ftelli64(pFile);
        CPacket head(4, (BYTE*)&data, 8);//第一个包的数据是文件长度，如果为0证明文件长度为0
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

//操作鼠标
int MouseEvent()
{
    MOUSEEVENT mouse;

    if (CServSocket::getInstance()->bGetMouseEvent(mouse))
    {
        DWORD nFlag = 0;
        switch (mouse.nButton)
        {
        case 0://左键
            nFlag = 1;
            break;
        case 1://右键
            nFlag = 2;
            break;
        case 2://中键
            nFlag = 4;
            break;
        case 4://没有按键
            nFlag = 8;
            break;
        }

        if (nFlag != 8)
        {
            SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
        }
        switch (mouse.nAction)
        {
        case 0://单击
            nFlag |= 0x10;
            break;
        case 1://双击
            nFlag |= 0x20;
            break;
        case 2://按住
            nFlag |= 0x40;
            break;
        case 3://放开
            nFlag |= 0x80;
            break;
        default:

            break;
        }

        TRACE("flag = %08x, x = %d, y = %d\r\n", nFlag, mouse.ptXY.x, mouse.ptXY.y);
        switch (nFlag)
        {
        case 0x21://左键双击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x11://左键单击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x41://左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81://左键松开
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x22://右键双击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x12://右键单击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x42://右键按住
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82://右键松开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x24://中键双击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x14://中键单击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x44://中键按住
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84://中键松开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x08://单纯鼠标移动
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break;
        }

        CPacket pack(5, NULL, 0);
        CServSocket::getInstance()->bSend(pack);
    }
    else
    {
        OutputDebugString(_T("获取鼠标操作参数失败！！"));
        return -1; 
    }

    return 0;
}

//发送屏幕截图
#include <atlimage.h>
int SendScreen()
{
    CImage screen;//C++封装的关于图像的类
    HDC hScreen = ::GetDC(NULL);//获取设备的上下文
    int nBitPixel = GetDeviceCaps(hScreen, BITSPIXEL);//获取设备的多个属性：得到位宽
    int nWidth = GetDeviceCaps(hScreen, HORZRES);//得到宽度
    int nHeight = GetDeviceCaps(hScreen, VERTRES);//得到高度

    screen.Create(nWidth, nHeight, nBitPixel);
    BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
    ReleaseDC(NULL, hScreen);

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//获取全局可移动的内存
    if (hMem == NULL)
    {
        return -1;
    }
    IStream* pStream = NULL;//建立一个内存流，利用Save的重载函数
    HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);//将内存流创建在全局可移动的内存上
    if(hRet == S_OK)
    {
        screen.Save(pStream, Gdiplus::ImageFormatPNG);//将屏幕保存到内存流
        LARGE_INTEGER begin = { 0 };
        pStream->Seek(begin, STREAM_SEEK_SET, NULL);//将内存流的指针设置到流的头部
        PBYTE pData = (PBYTE)GlobalLock(hMem);//将hMem与pStream lock传递给pData，
                                                //不lock hMem和pStream是分离的，读不到数据
        SIZE_T nSize = GlobalSize(hMem);
        CPacket pack(6, pData, nSize);//将读出来的内存数据打包
        CServSocket::getInstance()->bSend(pack);
        GlobalUnlock(hMem);
    }

    pStream->Release();
    GlobalFree(hMem);
    screen.ReleaseDC();

    /*
    DWORD tick = GetTickCount64();
    screen.Save(_T("test2020.png"), Gdiplus::ImageFormatPNG);
    TRACE("png %d\r\n", GetTickCount64() - tick);
    tick = GetTickCount64();
    screen.Save(_T("test2020.jpg"), Gdiplus::ImageFormatJPEG);
    TRACE("jpg %d\r\n", GetTickCount64() - tick);
    screen.ReleaseDC();
    */

    return 0;
}

#include "LockInfoDialog.h"
CLockInfoDialog dlg;
unsigned int threadid = 0;

unsigned _stdcall threadLockDlg(void* arg)
{
    TRACE("%s(%d): %d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
    dlg.Create(IDD_DIALOG_INFO, NULL);//非模态Dialog创建
    dlg.ShowWindow(SW_SHOW);//显示窗口
    CRect rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
    rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
    rect.bottom = LONG(rect.bottom * 1.07);
    dlg.MoveWindow(rect);//设置窗口显示大小
    CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
    if (pText)
    {
        CRect rtText;
        pText->GetWindowRect(rtText);
        int nWidth = rtText.Width();
        int x = (rect.right - nWidth) / 2;
        int nHeight = rtText.Height();
        int y = (rect.bottom - nHeight) / 2;
        pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
    }//将"联系管理员"文本置于屏幕正中央

    //窗口置顶
    dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    //不显示鼠标
    ShowCursor(FALSE);
    //隐藏任务栏
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);

    dlg.GetWindowRect(rect);
    rect.left = 0;
    rect.top = 0;
    rect.right = 1;
    rect.bottom = 1;
    ClipCursor(rect);//限制鼠标活动范围

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {//MFC编程是基于消息循环的，所以必须有这个循环对话框才显示
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_KEYDOWN && msg.wParam == 0x1B)//按下ESC退出
        {
            TRACE("msg: %08X, wparam: %08X, lparam: %08X\r\n",
                msg.message, msg.wParam, msg.lParam);
            break;
        }
    }

    //显示任务栏
	::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
    //恢复鼠标活动范围限制
    ClipCursor(NULL);
    //显示鼠标
	ShowCursor(TRUE);
    //销毁窗口
    dlg.DestroyWindow();

    _endthreadex(0);
    return 0;
}

int LockMachine()
{
    if (dlg.m_hWnd == NULL || dlg.m_hWnd == INVALID_HANDLE_VALUE)
	{
		//_beginthread(threadLockDlg, 0, NULL);//放到一个线程里，避免消息死循环接收不到unlock
        _beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadid);
        TRACE("threadid: %d\r\n", threadid);
    }

	CPacket pack(7, NULL, 0);
	CServSocket::getInstance()->bSend(pack);
	return 0;
}

int UnLockMachine()
{
    //前两个方法不可以的原因是LockMachine是用线程控制的，线程只能接收到自己线程的消息，
    // 所以需要用PostThreadMessage方法给对应线程发消息
	//dlg.SendMessage(WM_KEYDOWN, 0x1B, 00010001);
    //::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x1B, 00010001);
    PostThreadMessage(threadid, WM_KEYDOWN, 0x1B, 00010001);
	CPacket pack(8, NULL, 0);
	CServSocket::getInstance()->bSend(pack);
	return 0;
}

int DeleteLocalFile()
{
	std::string strPath;
	CServSocket::getInstance()->bGetFilePath(strPath);
    TCHAR sPath[MAX_PATH] = _T("");
    //mbstowcs(sPath, strPath.c_str(), strPath.size());中文容易乱码
    MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath, sizeof(sPath) / sizeof(TCHAR));
	DeleteFile(sPath);

    CPacket pack(9, NULL, 0);
    bool bRet = CServSocket::getInstance()->bSend(pack);
    TRACE("ret = %d", bRet);

    return 0;
}

int TestConnect()
{
    CPacket pack(1981, NULL, 0);
	bool bRet = CServSocket::getInstance()->bSend(pack);
    TRACE("Send ret = %d\r\n", bRet);
    
    return 0;
}

int ExcuteCmd(int nCmd)
{
    int nRet = 0;
	switch (nCmd)
	{//需求：处理文件
	case 1:// ==> 需要查看磁盘分区
        nRet = MakeDriverInfo();
		break;
	case 2:// ==> 需要查看指定目录下的文件
        nRet = MakeDirecoryInfo();
		break;
	case 3:// ==> 需要打开文件
        nRet = RunFile();
		break;
	case 4:// ==> 需要下载文件
        nRet = DownLoadFile();
		break;
	case 5:// ==> 需要操作鼠标
        nRet = MouseEvent();
		break;
	case 6:// ==> 需要发送屏幕内容 ==> 本质是发送屏幕的截图
        nRet = SendScreen();
		break;
	case 7:// ==> 需要锁住机器不让用户操纵
        nRet = LockMachine();
		break;
	case 8:
        nRet = UnLockMachine();
		break;
    case 9:
        nRet = DeleteLocalFile();
        break;
    case 1981:
        nRet = TestConnect();
        break;
	}
    return nRet;
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
            CServSocket*    pServer = CServSocket::getInstance();
            // 统计 accept 客户端失败的次数，连续失败太多就结束程序。
            int             nCount = 0;
            // 初始化监听 socket：创建地址、绑定 9527 端口、进入监听状态。
            if (!pServer->bInitSocket())
            {
                MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态"),
                    _T("网络初始化失败"), MB_OK | MB_ICONERROR);
                exit(0);
            }
            // 服务端主循环。
            // 只要单例对象还存在，就不断等待客户端接入并处理客户端命令。
            while (CServSocket::getInstance() != NULL)
            {
                // 等待客户端连接。
                // 如果没有客户端连接，bAcceptClient 内部的 accept 会阻塞等待。
                if (!pServer->bAcceptClient())
                {
                    if (nCount >= 3)
                    {
                        MessageBox(NULL, _T("多次无法正常接入用户，结束程序"),
                            _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                        exit(0);
                    }
                    MessageBox(NULL, _T("无法正常接入用户，自动重试"), 
                        _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                    nCount++;
                }
                TRACE("Accept Client return true\r\n");
                // 客户端连接成功后，进入命令处理逻辑。
                // 当前 dealCommand 里还没有真正解析命令，只是在循环 recv。
                int nRet = pServer->dealCommand();
                TRACE("deal commmand nRet = %d\r\n", nRet);
                if (nRet > 0)
                {
                    nRet = ExcuteCmd(nRet);
                    if (nRet != 0)
                    {
                        TRACE("执行命令失败：%d，ret = %d\r\n", pServer->getPacket().sCmd, nRet);
                    }
                    pServer->CloseClient();
                }
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

