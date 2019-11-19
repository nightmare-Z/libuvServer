#include "service.h"

Service::Service()
{
	this->using_raw = false;
}

bool Service::recvServiceCmd(Session* s, commandMessage* msg)
{
	return false;
}

bool Service::disconnectService(Session* s, int sType)
{
	return false;
}

bool Service::recvServiceRawHead(Session* s, Gateway2Server* msg)
{
	return true;
}
