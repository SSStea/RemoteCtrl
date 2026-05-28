
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
{
	UpdateData();//默认为true：把控件的值赋给成员变量；false：把成员变量的值赋给控件

	// TODO: 在此添加控件通知处理程序代码
	CClientSocket* pClient = CClientSocket::getInstance();
	bool bRet = pClient->bInitSocket(m_server_address, atoi((LPCTSTR)m_nPort));//TODO：返回值处理
	if (!bRet)
	{
		AfxMessageBox("网络初始化失败！");
		return -1;
	}
	CPacket pack(nCmd, pData, nLength);
	bRet = pClient->bSend(pack);
	TRACE("send ret = %d\r\n", bRet);
	int nRetCmd = pClient->dealCommand();
	TRACE("ack: %d\r\n", nRetCmd);

	if (bAutoClose)
	{
		pClient->CloseSocket();
	}
	return nRetCmd;
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	m_server_address = 0x7F000001;
	m_nPort = _T("9527");
	UpdateData(FALSE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRemoteClientDlg::OnBnClickedBtnTest()
{
	SendCommandPacket(1981);
}

void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	// TODO: 在此添加控件通知处理程序代码
	int nRet = SendCommandPacket(1);
	if (nRet == -1)
	{
		AfxMessageBox(_T("命令处理失败！！"));
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	std::string strDrivers = pClient->getPacket().strData;
	std::string dr;
	m_Tree.DeleteAllItems();

	for (size_t i = 0; i < strDrivers.size(); i++)
	{
		if (strDrivers[i] == ',')
		{
			dr.push_back(':');
			HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			m_Tree.InsertItem("", hTemp, TVI_LAST);//对获取的所有驱动都插入一个空的子目录
			dr.clear();
			continue;
		}
		dr.push_back(strDrivers[i]);
	}
}

void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);

	m_List.DeleteAllItems();

	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());

	pFILEINFO pInfo = (pFILEINFO)CClientSocket::getInstance()->getPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	while (pInfo->bHasNext)//向服务端请求目录时可能是对某个空目录请求，这样就不必处理了
	{
		TRACE("[%s] is dir %d\r\n", pInfo->szFileName, pInfo->bIsDirectory);
		if (!pInfo->bIsDirectory)
		{
			m_List.InsertItem(0, pInfo->szFileName);
		}
		int nRetCmd = pClient->dealCommand();
		TRACE("ack: %d\r\n", nRetCmd);
		if (nRetCmd < 0)
		{
			break;
		}
		pInfo = (pFILEINFO)CClientSocket::getInstance()->getPacket().strData.c_str();
	}

	pClient->CloseSocket();
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);//获取鼠标的全局位置
	m_Tree.ScreenToClient(&ptMouse);//把屏幕坐标转换为客户端坐标

	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);//获取鼠标选择的TreeItem
	if (hTreeSelected == NULL)
	{//如果双击的不是TreeItem就不做任何操作，返回
		return;
	}
	if (m_Tree.GetChildItem(hTreeSelected) == NULL)
	{//如果双击的TreeItem没有子目录说明是文件，返回
		return;
	}
	DeleteTreeChildrenItem(hTreeSelected);//每次双击删除上次双击生成的子目录，避免无限增长
	m_List.DeleteAllItems();

	CString strPath = GetPath(hTreeSelected);
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());

	pFILEINFO pInfo = (pFILEINFO)CClientSocket::getInstance()->getPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	int nCount = 0;
	while (pInfo->bHasNext)//向服务端请求目录时可能是对某个空目录请求，这样就不必处理了
	{
		TRACE("[%s] is dir %d\r\n", pInfo->szFileName, pInfo->bIsDirectory);
		if (pInfo->bIsDirectory)
		{
			if (CString(pInfo->szFileName) == "." ||
				CString(pInfo->szFileName) == "..")
			{//遇到"."和".."目录就只获取下一个但是不操作
				int nRetCmd = pClient->dealCommand();
				TRACE("ack: %d\r\n", nRetCmd);
				if (nRetCmd < 0)
				{
					break;
				}
				pInfo = (pFILEINFO)CClientSocket::getInstance()->getPacket().strData.c_str();
				continue;
			}

			HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
			m_Tree.InsertItem("", hTemp, TVI_LAST);
		}
		else
		{
			m_List.InsertItem(0, pInfo->szFileName);
		}
		nCount++;
		int nRetCmd = pClient->dealCommand();
		TRACE("ack: %d\r\n", nRetCmd);
		if (nRetCmd < 0)
		{
			break;
		}
		pInfo = (pFILEINFO)CClientSocket::getInstance()->getPacket().strData.c_str();
	}

	pClient->CloseSocket();
	TRACE("Count = %d\r\n", nCount);
}

CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet, strTemp;

	do 
	{
		strTemp = m_Tree.GetItemText(hTree);
		strRet = strTemp + '\\' + strRet;
		hTree = m_Tree.GetParentItem(hTree);
	} while (hTree != NULL);

	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do 
	{
		hSub = m_Tree.GetChildItem(hTree);
		if(hSub != NULL)
		{
			m_Tree.DeleteItem(hSub);
		}
	} while (hSub != NULL);
}

void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	LoadFileInfo();
}

void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	LoadFileInfo();
}

void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);
	int nListSelected = m_List.HitTest(ptList);
	if (nListSelected < 0)//说明HitTest点击测试没选中
	{
		return;
	}

	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);//把IDR_MENU_RCLICK装载到menu对象
	CMenu* pPopup = menu.GetSubMenu(0);//取子菜单的第一个
	if (pPopup != NULL)
	{
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
		//通过TrackPopupMenu把子菜单的第一个弹出来
	}
}

void CRemoteClientDlg::OnDownloadFile()
{
	// TODO: 在此添加命令处理程序代码
	int nListSelected = m_List.GetSelectionMark();//获取List控件选择的Item
	CString strFileName = m_List.GetItemText(nListSelected, 0);//Item的第一个信息是文件名字
	HTREEITEM hSelected = m_Tree.GetSelectedItem();//获取Tree控件选择的Item，是路径
	CString strFilePath = GetPath(hSelected) + strFileName;//将文件名与文件的存储路径拼起来获得文件的绝对路径
	TRACE("%s\r\n", LPCSTR(strFilePath));

	CFileDialog cFileDlg(FALSE, 
		"*", 
		strFileName, 
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, 
		NULL, 
		this);//开启一个保存文件的Dialog

	if (cFileDlg.DoModal() == IDOK)
	{
		FILE* pFile = fopen(cFileDlg.GetPathName(), "wb+");
		if (pFile == NULL)
		{
			AfxMessageBox("本地无权限保存该文件，或文件无法创建");
			return;
		}

		CClientSocket* pClient = CClientSocket::getInstance();
		do
		{
			int nRetCmd = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFilePath, strFilePath.GetLength());
			if (nRetCmd < 0)
			{
				AfxMessageBox("执行下载命令失败！！");
				TRACE("ret = %d\r\n", nRetCmd);
				break;
			}

			long long lFileLength = *(long long*)pClient->getPacket().strData.c_str();
			if (lFileLength == 0)
			{
				AfxMessageBox("文件长度为零，或着无法读取文件！！");
				break;
			}

			long long lCount = 0;
			while (lCount < lFileLength)
			{
				nRetCmd = pClient->dealCommand();
				if (nRetCmd < 0)
				{
					AfxMessageBox("传输失败！！");
					TRACE("传输失败：ret = %d", nRetCmd);
					break;
				}
				fwrite(pClient->getPacket().strData.c_str(), 1,
					pClient->getPacket().strData.size(), pFile);
				lCount += pClient->getPacket().strData.size();
			}
		}while (false);

		fclose(pFile);
		pClient->CloseSocket();
	}
	//TODO：大文件传输需要额外处理
}

void CRemoteClientDlg::OnDeleteFile()
{
	// TODO: 在此添加命令处理程序代码
	int nListSelected = m_List.GetSelectionMark();
	CString strFileName = m_List.GetItemText(nListSelected, 0);
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strFilePath = GetPath(hSelected) + strFileName;

	int nRetCmd = SendCommandPacket(9, true, (BYTE*)(LPCSTR)strFilePath, strFilePath.GetLength());
	
	if (nRetCmd < 0)
	{
		AfxMessageBox("删除文件命令执行失败！！");
	}
	else
	{
		LoadFileCurrent();
	}
}

void CRemoteClientDlg::OnRunFile()
{
	// TODO: 在此添加命令处理程序代码
	int nListSelected = m_List.GetSelectionMark();
	CString strFileName = m_List.GetItemText(nListSelected, 0);
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strFilePath = GetPath(hSelected) + strFileName;

	int nRetCmd = SendCommandPacket(3, true, (BYTE*)(LPCSTR)strFilePath, strFilePath.GetLength());
	if (nRetCmd < 0)
	{
		AfxMessageBox("打开文件命令执行失败！！");
	}
}
