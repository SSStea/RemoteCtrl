#pragma once
#include "afxdialogex.h"

#ifndef WM_SEND_ACK
#define WM_SEND_ACK		(WM_USER+2)//发送应答包
#endif

static const UINT_PTR TIMER_MOUSE_MOVE = 1;

// CWatchDialog 对话框

class CWatchDialog : public CDialog
{
	DECLARE_DYNAMIC(CWatchDialog)

public:
	CWatchDialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CWatchDialog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_WATCH };
#endif
public:
	int m_nObjWidth;
	int m_nObjHeight;

	CImage& getImage()
	{
		return m_image;
	}


protected:
	CImage m_image;//图像缓存

	CPoint m_ptLatestRemote;//鼠标的最后一个本地坐标转化的远程坐标
	bool   m_bMouseMoveDirty;//表示鼠标位置是否有更新、尚未发送。
	bool   m_bMouseMovePending;//表示上一次鼠标移动请求是否还在等待服务器应答。

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CPoint UserPoint2RemoteScreenPoint(CPoint& point, bool bIsScreen = false);

	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	CStatic m_picture;
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnStnClickedWatch();
	virtual void OnOK();
	afx_msg void OnBnClickedBtnLock();
	afx_msg void OnBnClickedBtnUnlock();
	afx_msg LRESULT OnHandleAckPkt(WPARAM wParam, LPARAM lParam);
	afx_msg void OnPaint();
};
