// WatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	m_nObjWidth = -1;
	m_nObjHeight = -1;
}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 50, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0)
	{
		CRemoteClientDlg* pParent = (CRemoteClientDlg*) GetParent();//获取父窗口
		if (pParent->bIsFull())
		{
			CRect rect;//定义一个矩形对象，用来保存 m_picture 控件的位置和大小信息

			if (m_nObjWidth == -1)
			{
				m_nObjWidth = pParent->getImage().GetWidth();
			}
			if (m_nObjHeight == -1)
			{
				m_nObjHeight = pParent->getImage().GetHeight();
			}

			// 获取 m_picture 控件在屏幕坐标中的矩形区域，这里主要使用它的宽度和高度
			m_picture.GetWindowRect(rect);
			pParent->getImage().StretchBlt(//StretchBlt会把图片拉伸到指定的目标区域大小
				m_picture.GetDC()->GetSafeHdc(), // 获取 m_picture 控件的 HDC，用于绘图
				0,								// 目标区域左上角 x 坐标
				0,								// 目标区域左上角 y 坐标
				rect.Width(),					// 目标绘制宽度，等于控件宽度
				rect.Height(),					// 目标绘制高度，等于控件高度
				SRCCOPY							// 直接复制源图像到目标区域
			);//将父窗口中保存的图片绘制到 m_picture 控件的设备上下文上
			m_picture.InvalidateRect(NULL);// 通知系统 m_picture 控件需要重绘：NULL表示整个控件区域都需要刷新
			pParent->getImage().Destroy();// 销毁父窗口中保存的图片资源，释放内存
			m_picture.ReleaseDC(m_picture.GetDC());//释放m_picture的DC，避免 GDI 资源泄漏
			pParent->setImageStatus();// 更新图片状态，例如标记当前图片已经处理完成
		}
	}
	CDialog::OnTimer(nIDEvent);
}

CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool bIsScreen)
{//client:800*450
	CRect clientRect;
	if(bIsScreen)//如果是屏幕坐标，OnLButtonDblClk等函数获取的是客户端坐标而不是屏幕坐标
	{
		ScreenToClient(&point);//将屏幕坐标point转化为客户端坐标
	}
	m_picture.GetWindowRect(&clientRect);//获取客户端的Rect：800*450
	int width0 = clientRect.Width();
	int height0 = clientRect.Height();
	int width = m_nObjWidth, height = m_nObjHeight;//远程机的Rect
	int x = point.x * width / width0;
	int y = point.y * height / height0;//远程机的坐标

	return CPoint(x, y);
}

void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);

		MOUSEEVENT event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 1;//双击

		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
	}
	CDialog::OnLButtonDblClk(nFlags, point);
}

void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		TRACE("x = %d, y = %d\r\n", point.x, point.y);
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		TRACE("x = %d, y = %d\r\n", point.x, point.y);

		MOUSEEVENT event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 2;//按下

		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
	}

	CDialog::OnLButtonDown(nFlags, point);
}

void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);

		MOUSEEVENT event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 3;//弹起

		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
	}

	CDialog::OnLButtonUp(nFlags, point);
}

void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);

		MOUSEEVENT event;
		event.ptXY = remote;
		event.nButton = 1;//右键
		event.nAction = 1;//双击

		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
	}

	CDialog::OnRButtonDblClk(nFlags, point);
}

void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);

		MOUSEEVENT event;
		event.ptXY = remote;
		event.nButton = 1;//右键
		event.nAction = 2;//按下， TODO：服务端要做对应修改

		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
	}

	CDialog::OnRButtonDown(nFlags, point);
}

void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);

		MOUSEEVENT event;
		event.ptXY = remote;
		event.nButton = 1;//右键
		event.nAction = 3;//弹起

		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
	}

	CDialog::OnRButtonUp(nFlags, point);
}

void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);

		MOUSEEVENT event;
		event.ptXY = remote;
		event.nButton = 4;
		event.nAction = 4;//移动

		//TODO:网络通信和Client对话框有耦合，这是一个设计隐患，想要通信必须要调用对话框
		//对话框是V层（视图层），通信是C层（控制层），对话框依赖通信：V ==> C是可以的，但是
		//如果通信却要依赖对话框：C ==> V这样是不可以的，后续需要改善
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
	}

	CDialog::OnMouseMove(nFlags, point);
}

void CWatchDialog::OnStnClickedWatch()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		CPoint point;
		GetCursorPos(&point);//这里获取的是屏幕坐标，所以下面用true

		CPoint remote = UserPoint2RemoteScreenPoint(point, true);

		MOUSEEVENT event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 0;//单击

		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
	}
}

void CWatchDialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	//CDialog::OnOK();
}

void CWatchDialog::OnBnClickedBtnLock()
{
	// TODO: 在此添加控件通知处理程序代码
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 7 << 1 | 1);
}

void CWatchDialog::OnBnClickedBtnUnlock()
{
	// TODO: 在此添加控件通知处理程序代码
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 8 << 1 | 1);
}
