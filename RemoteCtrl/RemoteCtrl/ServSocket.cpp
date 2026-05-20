#include "pch.h"
#include "ServSocket.h"

// 单例对象指针的静态成员定义。
// 这里只是初始化为空，真正创建对象发生在 getInstance 里。
CServSocket* CServSocket::m_Instance = NULL;

// 静态辅助对象。
// 它的构造/析构会分别触发单例创建和释放。
CServSocket::CHelper CServSocket::m_helper;

// 如果打开这一行，全局变量初始化时也会主动创建服务端单例。
// 现在已经有 m_helper 负责创建和释放，所以这里暂时不需要。
//CServSocket* pServer = CServSocket::getInstance();

