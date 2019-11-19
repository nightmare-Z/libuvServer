#ifndef __LUA_SERVICE_H__
#define __LUA_SERVICE_H__

#include "netbus/service.h"

int regist_service_export(lua_State* L);
int regist_releaseRaw_export(lua_State* L);

class lua_service :public Service
{
public:
	unsigned int luaRecvCmdHandle;
	unsigned int luaDisconHandle;
	unsigned int luaRecvRawHandle;

public:
	//bool表示 收到的时违法命令则干掉这个连接	false表示是违法命令
	virtual bool recvServiceCmd(Session* s, commandMessage* msg);

	virtual bool recvServiceRawHead(Session* s, Gateway2Server* msg);
	//关闭连接并同步到其他模块
	virtual bool disconnectService(Session* s, int sType);
};
#endif // !__LUA_SERVICE_H__