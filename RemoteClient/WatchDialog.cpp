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
