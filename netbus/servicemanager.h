#ifndef __SERVICEMANAGER_H__
#define __SERVICEMANAGER_H__

#include "netbus/service.h"

class ServiceManager
{
public:
	//初始化服务
	static void init();
	//注册服务   tpye ：user , system , default. 
	static bool registService(int type, Service* service);
	//执行服务中的命令
	static bool recvCmdMsg(Session* session, Gateway2Server* msg);
	//释放服务
	static void sessionDisconnect(Session* session);
};

#endif