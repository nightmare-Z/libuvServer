#include <string.h>

#include "session.h"
#include "service.h"
#include "protocolmanager.h"
#include "servicemanager.h"

#define MAX_SERVICE_NUM  50

static Service* gServiceSet[MAX_SERVICE_NUM];

void ServiceManager::init()
{
	memset(gServiceSet, 0, sizeof(gServiceSet));
}

bool ServiceManager::registService(int type, Service* service)
{
	if (type < 0 || type >= MAX_SERVICE_NUM)return false;
	//不为空  注册过了
	if (gServiceSet[type])return false;

	gServiceSet[type] = service;
	return true;
}

bool ServiceManager::recvCmdMsg(Session* session, Gateway2Server* msg)
{
	//没有命令
	if (gServiceSet[msg->serverType] == NULL)return false;
	bool ret = false;
	if (gServiceSet[msg->serverType]->using_raw)
	{
		return gServiceSet[msg->serverType]->recvServiceRawHead(session, msg);
	}

	commandMessage* cmd = NULL;
	//解密数据包
	if (protocolManager::decodeCmdMsg(msg->rawCmd, msg->rawLen, &cmd))
	{
		ret  = gServiceSet[msg->serverType]->recvServiceCmd(session, cmd);
		protocolManager::freeDecodeCmdMsg(cmd);
	}

	return ret;
}

void ServiceManager::sessionDisconnect(Session* session)
{
	for (int i = 0; i < MAX_SERVICE_NUM; i++)
	{
		if (gServiceSet[i] == NULL)continue;
		gServiceSet[i]->disconnectService(session, i);
	}
}
