#ifndef __SESSION_H__
#define __SESSION_H__

#include "netbus/protocolmanager.h"

class Session
{
public:
	//Asclient  1 则标识为服务器  0  则是客户端
	unsigned int as_client;
	unsigned int utag;
	unsigned int uid;
	unsigned int gid;

	Session() {
		//默认为客户端
		this->as_client = 0;
		this->utag = 0;
		this->uid = 0;
		this->gid = 0;
	}
public:
	virtual void close() = 0;
	virtual void sendData(unsigned char* body, int len) = 0;
	virtual const char* getAddress(int* clientPort) = 0;
	virtual void sendMessage(commandMessage* msg)=0;
	virtual void sendMessage(Gateway2Server* msg) = 0;
};

#endif