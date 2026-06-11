#pragma once
#include <map>
#include <atlimage.h>
#include <direct.h>
#include <io.h>
#include <list>
#include "ServSocket.h"
#include "EdoyunTool.h"
#include "LockInfoDialog.h"
#include "Resource.h"
#include "Packet.h"

class CCommand
{
public:
	CCommand();
	~CCommand();
	int ExcuteCmd(int nCmd, std::list<CPacket>& lstOutPacket, CPacket& inPacket);
    static void RunCommand(void* arg, int nStatus, std::list<CPacket>& lstOutPacket, CPacket& inPacket);

protected:
	typedef int (CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket&);//成员函数指针：命令处理函数
	std::map<int, CMDFUNC> m_mapFunction; //map表：从命令号和对应命令处理函数的映射

	CLockInfoDialog dlg;
	unsigned int threadid;

protected:
    //查看磁盘分区
    int MakeDriverInfo(std::list<CPacket>& lstOutPacket, CPacket& inPacket); //1==>A 2==>B 3==>C ... 26==>Z

    //查看指定目录下的文件
    int MakeDirecoryInfo(std::list<CPacket>& lstOutPacket, CPacket& inPacket);

    //运行文件
    int RunFile(std::list<CPacket>& lstOutPacket, CPacket& inPacket);

    //下载文件
    int DownLoadFile(std::list<CPacket>& lstOutPacket, CPacket& inPacket);

    //操作鼠标
    int MouseEvent(std::list<CPacket>& lstOutPacket, CPacket& inPacket);

    //发送屏幕截图
    int SendScreen(std::list<CPacket>& lstOutPacket, CPacket& inPacket);

	static unsigned _stdcall threadLockDlg(void* arg);

    void threadLockDlgMain();
    
    //threadLockDlg函数是为了创建线程而准备的函数，这里是使用_beginthreadex去创建的线程，
    //_beginthreadex的函数签名要求第三个参数必须是一个静态成员函数或一个全局函数。
    //所以这里将threadLockDlg声明成一个静态的，但静态函数又没有this指针，所以无法调用CCommand里的成员
    //函数、成员变量，所以在给创建线程传参的时候传了一个this指针进去（_beginthreadex的第四个参数），
    //而threadLockDlg的arg参数就是我们创建线程传的this指针，我们可以转换回CCommand*类型去调用这个类
    //里的我们想要调用的成员函数、成员变量
	int LockMachine(std::list<CPacket>& lstOutPacket, CPacket& inPacket);

	int UnLockMachine(std::list<CPacket>& lstOutPacket, CPacket& inPacket);

	int DeleteLocalFile(std::list<CPacket>& lstOutPacket, CPacket& inPacket);

	int TestConnect(std::list<CPacket>& lstOutPacket, CPacket& inPacket);
};

