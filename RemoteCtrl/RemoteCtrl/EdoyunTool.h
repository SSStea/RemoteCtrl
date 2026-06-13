#pragma once
class CEdoyunTool
{
public:
	static void Dump(BYTE* pData, size_t nSize);

	//打印当前线程最后一个错误消息
	static void ShowError();

	//打印错误(根据错误代码)
	static std::string GetErrInfo(int wsaErrCode);

	//判断当进程是是管理员运行
	static bool IsAdmin();

	//提权(切换为管理员用户)
	/*
	PC需设置以下信息：
		本地安全策略→本地策略→安全选项
			账户：管理员账户状态  启用
			账户：使用空密码的本地账户只允许进行控制台登录 禁用
	*/
	//切换为管理员用户
	static bool RunAsAdmin();

//设置开启自启
/*
开启启动的时候，程序的权限是跟随启动用户的
如果两者权限不一致，则会导致程序启动失败
开机启动对环境变量有影响，如果依赖dll(动态库)，则可能启动失败
解决方法：
方法一【复制这些dll到system32下面或者syswow64下面】system32下面，多是64为位程序 SysWOW64下面多是32位程序
方法二【使用静态库，而非动态库】
*/


//设置开机自启方法一：修改注册表(登录过程中启动) 
//开机自启注册表位置：计算机\HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Run
	/*
	步骤：
		1.将可执行文件复制到系统变量路径下 system32/syswow64
		2.打开注册表开机自启位置
		3.将系统变量文件夹下的可执行文件路径添加到注册表开启自启路径下

	*/
	static int WriteRefisterTable(const CString SystemPath);

	//设置开机自启方式2：写入自启动文件夹（用户登录之后启动）
	//自启文件夹打开方式：win+R 输入:shell:startup
	/*
		方法：直接将可执行文件复制到自启动文件夹内即可
	*/
	static int WriteStartupDir(const CString& startupPath);

	//用于MFC命令行项目初始化
	static bool Init();
};

