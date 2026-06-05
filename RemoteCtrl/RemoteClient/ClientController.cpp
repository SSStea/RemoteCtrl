#include "pch.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapMsgFunc;
CClientController* CClientController::m_Instance = NULL;

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

	return nullptr;
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

	WaitForSingleObject(hEvent, -1);
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
	return LRESULT();
}

LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}
