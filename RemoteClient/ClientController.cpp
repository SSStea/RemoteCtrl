#include "pch.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapMsgFunc;
CClientController* CClientController::m_Instance = NULL;
CClientController::CHelper CClientController::m_helper;

CClientController* CClientController::getInstance()
{
	if (m_Instance == NULL)
	{
		m_Instance = new CClientController();
		struct 
		{
			UINT	nMsg;
			MSGFUNC func;
		}MsgFuncs[] = {
			{WM_SEND_PACK, &CClientController::OnSendPack},
			{WM_SEND_DATA, &CClientController::OnSendData},
			{WM_SHOW_STATUS, &CClientController::OnShowStatus},
			{WM_SHOW_WATCHER, &CClientController::OnShowWatcher},
			{(UINT) - 1, NULL}
		};

		for (int i = 0; MsgFuncs[i].func != NULL; i++)
		{
			m_mapMsgFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));
		}
	}

	return m_Instance;
}

int CClientController::InitController()
{
	//使用的CreateThread创建的线程，返回线程的句柄、ID
	m_hThread = (HANDLE)_beginthreadex(
		NULL,
		0,
		&CClientController::threadMsgHandleEntry,
		this,
		0,
		&m_nThreadID);

	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);

	return 0;
}

INT_PTR CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}

LRESULT CClientController::SendMessage(MSG msg)
{
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent == NULL)
	{
		return -2;
	}
	MSGINFO info(msg);

	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)hEvent);

	WaitForSingleObject(hEvent, INFINITE);
	CloseHandle(hEvent);
	return info.result;
}


unsigned __stdcall CClientController::threadMsgHandleEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadMsgHandle();

	_endthreadex(0);
	return 0;
}

void CClientController::threadMsgHandle()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_SEND_MESSAGE)
		{
			MSGINFO* pMsg = (MSGINFO*)msg.wParam;
			HANDLE hEvent = (HANDLE)msg.lParam;

			auto it = m_mapMsgFunc.find(pMsg->msg.message);
			if (it != m_mapMsgFunc.end())
			{
				//执行消息处理函数
				pMsg->result = (this->*it->second)(pMsg->msg.message, pMsg->msg.wParam,
					pMsg->msg.lParam);
			}
			else
			{
				pMsg->result = -1;
			}
			SetEvent(hEvent);
		}
		else
		{
			auto it = m_mapMsgFunc.find(msg.message);
			if (it != m_mapMsgFunc.end())
			{
				(this->*it->second)(msg.message, msg.wParam, msg.lParam);
			}
		}
	}
}

LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket* pack = (CPacket*)wParam;
	return LRESULT();//pClient->bSendPkt(*pack);
}

LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	char* pBuffer = (char*)wParam;
	return LRESULT();//pClient->bSend(pBuffer, (int)lParam);
}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}

void CClientController::UpdataAddress(int nIP, int nPort)
{
	CClientSocket::getInstance()->UpdataAddress(nIP, nPort);
}

void CClientController::CloseSocket()
{
	CClientSocket::getInstance()->CloseSocket();
}

bool CClientController::SendCommandPacket(
	HWND hWnd,//收到数据包后，需要应答给哪个窗口
	int nCmd,
	BYTE* pData,
	size_t nLength,
	bool bIsAutoClosed,
	LPARAM lParam
)
{
	CPacket reqPkt(nCmd, pData, nLength);//请求包
	CClientSocket* pClient = CClientSocket::getInstance();
	
	return pClient->bSendPkt(hWnd, reqPkt, bIsAutoClosed, lParam);
}

int CClientController::DonwloadFile(CString strPath)
{
	CFileDialog cFileDlg(
		FALSE,
		NULL,
		strPath,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		NULL,
		&m_remoteDlg);//开启一个保存文件的Dialog

	if (cFileDlg.DoModal() == IDOK)
	{
		m_strRemoteFilePath = strPath;
		m_strLocalFilePath = cFileDlg.GetPathName();

		FILE* pFile = fopen(m_strLocalFilePath, "wb+");
		if (pFile == NULL)
		{
			AfxMessageBox("本地无权限保存该文件，或文件无法创建");
			return -1;
		}

		bool bRet = SendCommandPacket(
			m_remoteDlg,
			4,
			(BYTE*)(LPCSTR)m_strRemoteFilePath,
			m_strRemoteFilePath.GetLength(),
			false,
			(LPARAM)pFile
		);
		if (!bRet)
		{
			AfxMessageBox("下载命令发送失败");
			fclose(pFile);
		}

		m_remoteDlg.BeginWaitCursor();
		m_statusDlg.m_info.SetWindowText(_T("命令正在执行中！！"));
		m_statusDlg.ShowWindow(SW_SHOW);
		m_statusDlg.CenterWindow(&m_remoteDlg);
		m_statusDlg.SetActiveWindow();
	}

	return 0;
}

void CClientController::DonwloadFileEnd()
{
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("下载完成！！"), _T("完成"));
}

void CClientController::StartWatchScreen()
{
	m_watchDlg.DoModal();
}

