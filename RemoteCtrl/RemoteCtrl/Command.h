#pragma once
#include <map>
#include <atlimage.h>
#include <direct.h>
#include <io.h>
#include <list>
#include "ServSocket.h"
#include "EdoyunTool.h"
#include "LockInfoDialog.h"
#include "Resource.h"
#include "Packet.h"

class CCommand
{
public:
	CCommand();
	~CCommand() {}
	int ExcuteCmd(int nCmd, std::list<CPacket>& lstOutPacket, CPacket& inPacket);
    static void RunCommand(void* arg, int nStatus, std::list<CPacket>& lstOutPacket, CPacket& inPacket)
    {
        CCommand* thiz = (CCommand*)arg;
        if(nStatus > 0)//状态大于0即获取到要执行的命令
        {
            int nRet = thiz->ExcuteCmd(nStatus, lstOutPacket, inPacket);
			if (nRet != 0)
			{
				TRACE("执行命令失败：%d，ret = %d\r\n", nStatus, nRet);
			}
        }
        else
        {
            MessageBox(NULL, _T("无法正常接入用户，自动重试"),
                        _T("接入用户失败！"), MB_OK | MB_ICONERROR);
        }
    }

protected:
	typedef int (CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket&);//成员函数指针：命令处理函数
	std::map<int, CMDFUNC> m_mapFuntion; //map表：从命令号和对应命令处理函数的映射

	CLockInfoDialog dlg;
	unsigned int threadid;

protected:
    //查看磁盘分区
    int MakeDriverInfo(std::list<CPacket>& lstOutPacket, CPacket& inPacket)//1==>A 2==>B 3==>C ... 26==>Z
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
        lstOutPacket.push_back(pack);

        CEdoyunTool::Dump((BYTE*)pack.Data(), pack.Size());

        return 0;
    }

    //查看指定目录下的文件
    int MakeDirecoryInfo(std::list<CPacket>& lstOutPacket, CPacket& inPacket)
    {
        std::string strPath = inPacket.strData;

        if (_chdir(strPath.c_str()) != 0)
        {
            FILEINFO fInfo;
            fInfo.bHasNext = FALSE;
            CPacket pack(2, (BYTE*)&fInfo, sizeof(fInfo));
            lstOutPacket.push_back(pack);

            OutputDebugString(_T("无权限访问目录！"));
            return -2;
        }

        _finddata_t fData;
        intptr_t hFind = 0;
        if ((hFind = _findfirst("*", &fData)) == -1)
        {
            OutputDebugString(_T("未找到任何文件！"));
            FILEINFO fInfo;
            fInfo.bHasNext = FALSE;
            CPacket pack(2, (BYTE*)&fInfo, sizeof(fInfo));
            lstOutPacket.push_back(pack);

            return -3;
        }
        do {
            FILEINFO fInfo;
            fInfo.bIsDirectory = ((fData.attrib & _A_SUBDIR) != 0);
            //(fData.attrib & _A_SUBDIR) != 0 ==>TRUE
            memcpy(fInfo.szFileName, fData.name, strlen(fData.name));
            TRACE("%s\r\n", fInfo.szFileName);

            CPacket pack(2, (BYTE*)&fInfo, sizeof(fInfo));
            lstOutPacket.push_back(pack);//获取一个文件就压入一个
        } while (!_findnext(hFind, &fData));

        FILEINFO fInfo;
        fInfo.bHasNext = FALSE;//告诉控制端没有下一个文件了，不必继续等待
        CPacket pack(2, (BYTE*)&fInfo, sizeof(fInfo));
        lstOutPacket.push_back(pack);

        return 0;
    }

    //运行文件
    int RunFile(std::list<CPacket>& lstOutPacket, CPacket& inPacket)
    {
        std::string strPath = inPacket.strData;

        ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);

        CPacket pack(3, NULL, 0);
        lstOutPacket.push_back(pack);

        return 0;
    }

    //下载文件
    int DownLoadFile(std::list<CPacket>& lstOutPacket, CPacket& inPacket)
    {
		std::string strPath = inPacket.strData;
		long long data = 0;
        FILE* pFile = NULL;
        errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
        if (err != 0)
        {
            CPacket pack(4, (BYTE*)&data, 8);//第一个包的数据是文件长度，如果为0证明打开失败
            lstOutPacket.push_back(pack);
            return -1;
        }

        if (pFile != NULL)
        {
            fseek(pFile, 0, SEEK_END);
            data = _ftelli64(pFile);
            CPacket head(4, (BYTE*)&data, 8);//第一个包的数据是文件长度，如果为0证明文件长度为0
            fseek(pFile, 0, SEEK_SET);
            lstOutPacket.push_back(head);

            char buffer[1024] = "";
            size_t rlen = 0;
            do {
                rlen = fread(buffer, 1, 1024, pFile);
                CPacket pack(4, (BYTE*)buffer, rlen);
                lstOutPacket.push_back(pack);
            } while (rlen >= 1024);

            fclose(pFile);
        }
        CPacket pack(4, NULL, 0);
        lstOutPacket.push_back(pack);

        return 0;
    }

    //操作鼠标
    int MouseEvent(std::list<CPacket>& lstOutPacket, CPacket& inPacket)
    {
        MOUSEEVENT mouse;
        memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEVENT));

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
        lstOutPacket.push_back(pack);

        return 0;
    }

    //发送屏幕截图
    int SendScreen(std::list<CPacket>& lstOutPacket, CPacket& inPacket)
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
        if (hRet == S_OK)
        {
            screen.Save(pStream, Gdiplus::ImageFormatPNG);//将屏幕保存到内存流
            LARGE_INTEGER begin = { 0 };
            pStream->Seek(begin, STREAM_SEEK_SET, NULL);//将内存流的指针设置到流的头部
            PBYTE pData = (PBYTE)GlobalLock(hMem);//将hMem与pStream lock传递给pData，
            //不lock hMem和pStream是分离的，读不到数据
            SIZE_T nSize = GlobalSize(hMem);
            CPacket pack(6, pData, nSize);//将读出来的内存数据打包
            lstOutPacket.push_back(pack);
            GlobalUnlock(hMem);
        }

        pStream->Release();
        GlobalFree(hMem);
        screen.ReleaseDC();

        
        //DWORD tick = GetTickCount64();
        //screen.Save(_T("test2020.png"), Gdiplus::ImageFormatPNG);
        //TRACE("png %d\r\n", GetTickCount64() - tick);
        //tick = GetTickCount64();
        //screen.Save(_T("test2020.jpg"), Gdiplus::ImageFormatJPEG);
        //TRACE("jpg %d\r\n", GetTickCount64() - tick);
        //screen.ReleaseDC();
        

        return 0;
    }

	static unsigned _stdcall threadLockDlg(void* arg)
	{
        CCommand* thiz = (CCommand*)arg;
        thiz->threadLockDlgMain();

		_endthreadex(0);
		return 0;
	}

    void threadLockDlgMain()
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
    }
    
    //threadLockDlg函数是为了创建线程而准备的函数，这里是使用_beginthreadex去创建的线程，
    //_beginthreadex的函数签名要求第三个参数必须是一个静态成员函数或一个全局函数。
    //所以这里将threadLockDlg声明成一个静态的，但静态函数又没有this指针，所以无法调用CCommand里的成员
    //函数、成员变量，所以在给创建线程传参的时候传了一个this指针进去（_beginthreadex的第四个参数），
    //而threadLockDlg的arg参数就是我们创建线程传的this指针，我们可以转换回CCommand*类型去调用这个类
    //里的我们想要调用的成员函数、成员变量
	int LockMachine(std::list<CPacket>& lstOutPacket, CPacket& inPacket)
	{
		if (dlg.m_hWnd == NULL || dlg.m_hWnd == INVALID_HANDLE_VALUE)
		{
			//_beginthread(threadLockDlg, 0, NULL);//放到一个线程里，避免消息死循环接收不到unlock
			_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);
			TRACE("threadid: %d\r\n", threadid);
		}

		CPacket pack(7, NULL, 0);
        lstOutPacket.push_back(pack);
		return 0;
	}

	int UnLockMachine(std::list<CPacket>& lstOutPacket, CPacket& inPacket)
	{
		//前两个方法不可以的原因是LockMachine是用线程控制的，线程只能接收到自己线程的消息，
		// 所以需要用PostThreadMessage方法给对应线程发消息
		//dlg.SendMessage(WM_KEYDOWN, 0x1B, 00010001);
		//::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x1B, 00010001);
		PostThreadMessage(threadid, WM_KEYDOWN, 0x1B, 00010001);
		CPacket pack(8, NULL, 0);
        lstOutPacket.push_back(pack);
		return 0;
	}

	int DeleteLocalFile(std::list<CPacket>& lstOutPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData;
		TCHAR sPath[MAX_PATH] = _T("");
		//mbstowcs(sPath, strPath.c_str(), strPath.size());中文容易乱码
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), (int)strPath.size(), sPath, sizeof(sPath) / sizeof(TCHAR));
		DeleteFile(sPath);

		CPacket pack(9, NULL, 0);
        lstOutPacket.push_back(pack);

		return 0;
	}

	int TestConnect(std::list<CPacket>& lstOutPacket, CPacket& inPacket)
	{
		CPacket pack(1981, NULL, 0);
        lstOutPacket.push_back(pack);

		return 0;
	}
};

