#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"
#include "Resource.h"
#include "EdoyunTool.h"
#include <map>

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

	//更新网络服务器的地址、端口
	void UpdataAddress(int nIP, int nPort);

	//处理命令
	int dealCommand();

	//关闭套接字
	void CloseSocket();

	//发送命令包
	//1 查看磁盘分区 2 查看指定目录下文件 3 打开文件 4 下载文件
	//5 操作鼠标 6 发送屏幕内容 7 锁住机器 8 解锁 9 删除文件
	//返回值是状态，true为成功，false失败
	bool SendCommandPacket(
		HWND hWnd,
		int nCmd,
		BYTE* pData = NULL,
		size_t nLength = 0,
		bool bIsAutoClosed = true
	);

	//将数据装填进图像
	int loadImage(CImage& image);

	int DonwloadFile(CString strPath);

	void StartWatchScreen();

protected:
	CClientController():m_statusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg)
	{
		m_hThreadDownload	= INVALID_HANDLE_VALUE;
		m_hThreadWatch		= INVALID_HANDLE_VALUE;
		m_hThread			= INVALID_HANDLE_VALUE;
		m_bIsClosed			= true;
		m_nThreadID			= -1;
	}

	~CClientController()
	{
		WaitForSingleObject(m_hThread, 100);
	}

	static unsigned __stdcall threadMsgHandleEntry(void* arg);
	void threadMsgHandle();

	static void threadDownloadFileEntry(void* arg);
	void threadDownloadFile();

	static void threadWatchScreenEntry(void* arg);
	void threadWatchScreen();

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

	HANDLE				m_hThreadDownload;
	CString				m_strRemoteFilePath;
	CString				m_strLocalFilePath;

	HANDLE				m_hThreadWatch;
	bool				m_bIsClosed;//监视是否关闭

	//控制单例
	static CClientController* m_Instance;
	class CHelper
	{
	public:
		// 程序启动时，静态成员 m_helper 会先构造。
		// 这里主动调用 getInstance，让 Controller 管理对象提前创建。
		CHelper()
		{
			//m_helper是静态成员，构造会先于main函数构造，窗口类的构造函数会调用theApp的
			// main构造，由于main函数还没有构造完，此时先构造窗口类就会产生错误
			//CClientController::getInstance();
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

