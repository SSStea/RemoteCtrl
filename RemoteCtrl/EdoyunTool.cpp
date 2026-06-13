#include "pch.h"
#include "EdoyunTool.h"

void CEdoyunTool::Dump(BYTE* pData, size_t nSize)
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

void CEdoyunTool::ShowError()
{
	LPCWSTR lpMessageBuf = NULL;
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&lpMessageBuf, 0, NULL);

	OutputDebugString(lpMessageBuf);
	MessageBox(NULL, lpMessageBuf, TEXT("程序错误"), MB_OK | MB_ICONERROR);
	LocalFree((HLOCAL)lpMessageBuf);
}

std::string CEdoyunTool::GetErrInfo(int wsaErrCode)
{
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL, wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);

	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}

bool CEdoyunTool::IsAdmin()
{
	//拿到当前进程的令牌 Token
	HANDLE hToken = NULL;//令牌
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		ShowError();
		return false;
	}

	//获取提权信息
	TOKEN_ELEVATION eve;
	DWORD len = 0;
	if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE)
	{
		ShowError();
		return false;
	}
	CloseHandle(hToken);

	//判断是否提权
	if (len == sizeof(eve))
	{
		return eve.TokenIsElevated;//>0 提权  ==0 没有提权
	}
	printf("length of tokeninformation is %d\r\n", len);
	return false;
}

bool CEdoyunTool::RunAsAdmin()
{
	// 获取当前程序路径  
	WCHAR sPath[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, sPath, MAX_PATH); // 获取当前可执行文件路径  

	// 以管理员身份运行一个进程
	STARTUPINFOW si = { 0 };
	si.cb = sizeof(STARTUPINFOW);
	PROCESS_INFORMATION pi = { 0 };

	BOOL ret = CreateProcessWithLogonW((LPCWSTR)"Administrator", NULL, NULL, LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
	//MessageBox(NULL, (LPCSTR)sPath, TEXT("用户状态"), 0);
	if (!ret) {
		//MessageBox(NULL, TEXT("创建进程失败"), TEXT("程序错误"), MB_OK | MB_ICONERROR);
		ShowError();
		return false;
	}

	//MessageBox(NULL, TEXT("进程创建成功"), TEXT("用户状态"), 0);

	// 等待创建进程运行结束  
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return true;
}

int CEdoyunTool::WriteRefisterTable(const CString SystemPath)
{
	if (PathFileExists(SystemPath))return 0;//文件已经存在

	//可执行文件复制到系统变量文件夹中  system32/syswow64
	char exePath[MAX_PATH] = "";//当前可执行文件路径
	GetModuleFileName(NULL, (LPWSTR)exePath, MAX_PATH);//获得当前可执行文件路径
	bool ret = CopyFile((LPWSTR)exePath, SystemPath, FALSE);//可执行文件复制到系统变量文件夹中
	if (ret == FALSE)
	{
		MessageBox(NULL, TEXT("复制文件夹失败，是否权限不足?\r\n"), TEXT("错误"), MB_ICONERROR | MB_TOPMOST);
		return -1;
	}

	//打开注册表开启自启位置
	CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");//注册表开机自启路径
	HKEY hKey = NULL;
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
	if (ret != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		MessageBox(NULL, TEXT("打开注册表失败! 是否权限不足?\r\n"), TEXT("错误"), MB_ICONERROR | MB_TOPMOST);
		return -2;
	}

	//将系统变量文件夹下的可执行文件路径添加到注册表开启自启路径下
	ret = RegSetValueEx(hKey, TEXT("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)SystemPath, SystemPath.GetLength() * sizeof(TCHAR));
	if (ret != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		MessageBox(NULL, TEXT("注册表添加文件失败 是否权限不足?\r\n"), TEXT("错误"), MB_ICONERROR | MB_TOPMOST);
		return -3;
	}
	RegCloseKey(hKey);
	return 0;
}

int CEdoyunTool::WriteStartupDir(const CString& startupPath)
{
	if (PathFileExists(startupPath))return 0;//文件已经存在
	TCHAR exePath[MAX_PATH] = TEXT("");
	GetModuleFileName(NULL, exePath, MAX_PATH);//获取当前可执行文件路径

	BOOL ret = CopyFile(exePath, startupPath, FALSE);//可执行文件放到开机自启文件夹中
	if (ret == FALSE)
	{
		MessageBox(NULL, TEXT("复制文件夹失败，是否权限不足?\r\n"), TEXT("错误"), MB_ICONERROR | MB_TOPMOST);
		return -1;
	}
	return 0;
}

bool CEdoyunTool::Init()
{
	//通用：用于MFC命令行项目初始化
	HMODULE hModule = ::GetModuleHandle(nullptr);
	if (hModule == nullptr)
	{
		// TODO: 更改错误代码以符合需要
		wprintf(L"错误: GetModuleHandle 失败\n");
		return false;
	}

	if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
	{
		// TODO: 在此处为应用程序的行为编写代码。
		wprintf(L"错误: MFC 初始化失败\n");
		return false;
	}
	return true;
}
