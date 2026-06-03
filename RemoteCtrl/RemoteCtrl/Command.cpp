#include "pch.h"
#include "Command.h"

CCommand::CCommand():threadid(0)
{
	struct 
	{
		int nCmd;
		CMDFUNC func;
	}data[] = {
		{1, &CCommand::MakeDriverInfo},
		{2, &CCommand::MakeDirecoryInfo},
		{3, &CCommand::RunFile},
		{4, &CCommand::DownLoadFile},
		{5, &CCommand::MouseEvent},
		{6, &CCommand::SendScreen},
		{7, &CCommand::LockMachine},
		{8, &CCommand::UnLockMachine},
		{9, &CCommand::DeleteLocalFile},
		{1981, &CCommand::TestConnect},
		{-1, NULL}
	};//一个数组结构体

	for (int i = 0; data[i].nCmd != -1; i++)
	{
		//将数组结构体中的命令和对应命令处理函数的映射存到map表中
		m_mapFuntion.insert(std::pair<int, CMDFUNC>(data[i].nCmd, data[i].func));
	}
}

int CCommand::ExcuteCmd(int nCmd, std::list<CPacket>& lstOutPacket, CPacket& inPacket)
{
	auto it = m_mapFuntion.find(nCmd);
	if (it == m_mapFuntion.end())
	{
		return -1;
	}

	return (this->*it->second)(lstOutPacket, inPacket);
	//迭代器it的second是类的成员函数指针
}
