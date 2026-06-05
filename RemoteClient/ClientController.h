#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"
#include "Resource.h"
#include <map>

#define WM_SEND_PACK	(WM_USER+1)//发送包数据
#define WM_SEND_DATA	(WM_USER+2)//发送数据
#define WM_SHOW_STATUS	(WM_USER+3)//展示状态
#define WM_SHOW_WATCHER	(WM_USER+4)//远程监控
#define WM_SEND_MESSAGE (WM_USER+0x1000) //自定义消息处理

class CClientController
{
public:
	//获取全局唯一对象，控制的单例
	static CClientController* getInstance();

	//初始化
	int InitController();
	//启动
	INT_PTR Invoke(CWnd*& pMainWnd);

	//发送消息
	LRESULT SendMessage(MSG msg);

protected:
	CClientController():m_statusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg)
	{
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
	}

	~CClientController()
	{
		WaitForSingleObject(m_hThread, 100);
	}

	static unsigned __stdcall threadMsgHandleEntry(void* arg);
	void threadMsgHandle();

	static void releaseInstance()
	{
		if (m_Instance != NULL)
		{
			delete m_Instance;
			m_Instance = NULL;
		}
	}

	LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	typedef struct MsgInfo
	{
		MsgInfo(MSG m)
		{
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}
		MsgInfo(const MsgInfo& m)
		{
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));
		}
		MsgInfo operator=(const MsgInfo& m)
		{
			if (this != &m)
			{
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));
			}
			return *this;
		}
		MSG		msg;
		LRESULT result;
	} MSGINFO;

	typedef LRESULT(CClientController::* MSGFUNC)(UINT, WPARAM, LPARAM);
	static std::map<UINT, MSGFUNC> m_mapMsgFunc;

	CWatchDialog		m_watchDlg;
	CRemoteClientDlg	m_remoteDlg;
	CStatusDlg			m_statusDlg;

	HANDLE				m_hThread;
	unsigned			m_nThreadID;

	//控制单例
	static CClientController* m_Instance;
	class CHelper
	{
	public:
		// 程序启动时，静态成员 m_helper 会先构造。
		// 这里主动调用 getInstance，让 Controller 管理对象提前创建。
		CHelper()
		{
			CClientController::getInstance();
		}

		// 程序结束时，静态成员 m_helper 会析构。
		// 这里释放单例对象，完成资源清理。
		~CHelper()
		{
			CClientController::releaseInstance();
		}
	};

	// 辅助释放单例的静态对象。
	static CHelper m_helper;
};

