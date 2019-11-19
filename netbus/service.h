#ifndef __SERVICE_H__
#define __SERVICE_H__

#include "netbus/session.h"

class Service
{
public:
	bool using_raw;
	Service();
public:
	//bool表示 收到的时违法命令则干掉这个连接	false表示是违法命令
	virtual bool recvServiceCmd(Session* s, commandMessage* msg);
	//关闭连接并同步到其他模块
	virtual bool disconnectService(Session* s , int sType);

	virtual bool recvServiceRawHead(Session* s, struct Gateway2Server* msg);
};

#endif