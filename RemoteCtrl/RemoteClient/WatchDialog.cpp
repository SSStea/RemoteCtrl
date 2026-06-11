// WatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"
#include "ClientController.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CImage& CWatchDialog::getImage()
{
		return m_image;
	}

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	m_nObjWidth = -1;
	m_nObjHeight = -1;
	m_bMouseMoveDirty = false;
	m_bMouseMovePending = false;
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
	ON_WM_PAINT()
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
	ON_MESSAGE(WM_SEND_ACK, &CWatchDialog::OnHandleAckPkt)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	//SetTimer(0, 50, NULL);
	SetTimer(TIMER_MOUSE_MOVE, 33, NULL);//鼠标移动的定时器，约30hz

	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 6, NULL, 0, false);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_MOUSE_MOVE &&
		m_bMouseMoveDirty &&
		!m_bMouseMovePending)
	{
		MOUSEEVENT event;
		event.ptXY = m_ptLatestRemote;
		event.nButton = 4;
		event.nAction = 4;

		bool bSent = CClientController::getInstance()->SendCommandPacket(
			GetSafeHwnd(),
			5,
			reinterpret_cast<BYTE*>(&event),
			sizeof(event)
		);

		if (bSent)
		{
			m_bMouseMoveDirty = false;
			m_bMouseMovePending = true;
		}
	}

	CDialog::OnTimer(nIDEvent);
}

CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool bIsScreen)
{//client:800*450
	CRect clientRect;
	if(bIsScreen)//如果是屏幕坐标，OnLButtonDblClk等函数获取的是客户端坐标而不是屏幕坐标
	{
		m_picture.ScreenToClient(&point);//将屏幕坐标point转化为客户端坐标
	}
	else
	{
		ClientToScreen(&point);//先将客户端左边转为全局屏幕坐标
		m_picture.ScreenToClient(&point);//再将全局的屏幕坐标转为图像缓存的客户端坐标
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

		CClientController::getInstance()->SendCommandPacket(
			GetSafeHwnd(), 
			5, 
			(BYTE*)&event, 
			sizeof(event)
		);
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

		CClientController::getInstance()->SendCommandPacket(
			GetSafeHwnd(),
			5,
			(BYTE*)&event,
			sizeof(event)
		);
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

		CClientController::getInstance()->SendCommandPacket(
			GetSafeHwnd(),
			5,
			(BYTE*)&event,
			sizeof(event)
		);
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

		CClientController::getInstance()->SendCommandPacket(
			GetSafeHwnd(),
			5,
			(BYTE*)&event,
			sizeof(event)
		);
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

		CClientController::getInstance()->SendCommandPacket(
			GetSafeHwnd(),
			5,
			(BYTE*)&event,
			sizeof(event)
		);
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

		CClientController::getInstance()->SendCommandPacket(
			GetSafeHwnd(),
			5,
			(BYTE*)&event,
			sizeof(event)
		);
	}

	CDialog::OnRButtonUp(nFlags, point);
}

void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)//只记录最新坐标
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CRect rectPicture;
	m_picture.GetWindowRect(&rectPicture);
	ScreenToClient(&rectPicture);

	if (rectPicture.PtInRect(point) &&
		m_nObjWidth > 0 &&
		m_nObjHeight > 0)
	{
		m_ptLatestRemote = UserPoint2RemoteScreenPoint(point);
		m_bMouseMoveDirty = true;
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

		CClientController::getInstance()->SendCommandPacket(
			GetSafeHwnd(),
			5,
			(BYTE*)&event,
			sizeof(event)
		);
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
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 7);
}

void CWatchDialog::OnBnClickedBtnUnlock()
{
	// TODO: 在此添加控件通知处理程序代码
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 8);
}

void CWatchDialog::OnPaint()
{
	CPaintDC dc(this);

	if (m_image.IsNull() || !::IsWindow(m_picture.GetSafeHwnd()))
	{
		return;
	}

	CRect rect;//定义一个矩形对象，用来保存 m_picture 控件的位置和大小信息
	// 获取 m_picture 控件在屏幕坐标中的矩形区域，这里主要使用它的宽度和高度
	m_picture.GetWindowRect(rect);
	ScreenToClient(&rect);

	if(dc != NULL)
	{
		m_image.StretchBlt(//StretchBlt会把图片拉伸到指定的目标区域大小
			dc.GetSafeHdc(), // 获取 m_picture 控件的 HDC，用于绘图
			0,								// 目标区域左上角 x 坐标
			0,								// 目标区域左上角 y 坐标
			rect.Width(),					// 目标绘制宽度，等于控件宽度
			rect.Height(),					// 目标绘制高度，等于控件高度
			SRCCOPY							// 直接复制源图像到目标区域
		);//将父窗口中保存的图片绘制到 m_picture 控件的设备上下文上
	}
	TRACE("更新图片完成 %d %d %08X\r\n", m_nObjWidth, m_nObjHeight, (HBITMAP)m_image);
}

LRESULT CWatchDialog::OnHandleAckPkt(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || lParam == -2)
	{

	}
	else if (lParam == 1)
	{

	}
	else
	{
		if (wParam == NULL)
		{
			return 0;
		}
		CPacket pAckPkt = *(CPacket*)wParam;
		delete (CPacket*)wParam;

		if (pAckPkt.Size() < 0)
		{
			return 0;
		}
		switch (pAckPkt.sCmd)
		{
		case 6:
			{
				CImage imageNew;
				HRESULT hRet = CEdoyunTool::nBytes2Image(imageNew, pAckPkt.strData);
				if(hRet != S_OK || imageNew.IsNull())
				{
					TRACE("图像设置失败！！ %d", hRet);
					break;
				}

				m_image.Destroy();
				m_image.Attach(imageNew.Detach());

				m_nObjHeight = m_image.GetHeight();
				m_nObjWidth = m_image.GetWidth();

				Invalidate(FALSE);
				
				Sleep(100);
				CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 6, NULL, 0, false);
			}
			break;
		case 5:
			m_bMouseMovePending = false;
			break;
		case 7:
		case 8:
		default:
			break;
		}

	}

	return 0;
}
