#ifndef __SESSION_UV_H__
#define __SESSION_UV_H__

#include "session.h"

//每一个客户都接收数据的大小
#define RECVBUF 4096 

//分别连接进来的是哪种连接
enum
{
	TCP_SOCKET,
	WEB_SOCKET
};

//分配session用到的内存池总空间
void initSessionAllocer();

class Session_uv :public Session
{
public:
	uv_tcp_t tcpHandler;
	char cipAddr[32];
	int cPort;

	uv_shutdown_t uvShotdown;
	bool isShotdown;
public:
	char recvBuf[RECVBUF];
	int received;
	int socketType;

	char* longPkg;
	int longPkgSize;

public:
	int wsShake;

private:
	void init();
	void exit();

public:
	static Session_uv* create();
	static void destroy(Session_uv* s);
	void* operator new(size_t size);
	void operator delete(void* mem);

public:
	virtual void close();
	virtual void sendData(unsigned char* body, int len);
	virtual const char* getAddress(int* clientPort);
	virtual void sendMessage(commandMessage* msg);
	virtual void sendMessage(Gateway2Server* msg);

public:
	//udp session
	uv_udp_t udpHandler;
	int udpStatus;
	const sockaddr* udpAdddr;
	char sipAddr[32];
	int sPort;
};

#endif