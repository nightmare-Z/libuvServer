#ifndef __NETBUS_H__
#define __NETBUS_H__

class Session;

class NetBus {
public:

	//全局唯一实例
	static NetBus* instance();

public:
	//服务器初始化装载
	void init();

	//服务器启动
	void run();

	//tcp连接开启
	void startTcpServer(const char* ip, int port);

	//web连接开启
	void startWebServer(const char* ip, int port);

	//udp开启
	void startUdpServer(const char* ip, int port);

	void tcp_connect(const char* ip, int port, void(*on_connected)(int err, Session* s, void* udata), void* udata);

};

#endif