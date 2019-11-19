#include "uv.h"
#include "session_uv.h"
#include "webprotocol.h"
#include "tcpprotocol.h"
#include "protocolmanager.h"
#include "servicemanager.h"
#include "utils/logger.h"
#include "netbus.h"
#include "utils/Cache.h"
//唯一NetBus服务器唯一实例
static NetBus g_NetBus;

struct udpRecvInfo {
	char* recvBuf;
	int maxRecvLen;
};

extern "C"
{
	//客户端发过来的执行命令
	static void RecCommand(Session* session, unsigned char* cmd, int len)
	{
		
		Gateway2Server msg;
		if (protocolManager::decodeMsgHead(cmd, len, &msg))
		{
			if (!ServiceManager::recvCmdMsg(session, &msg))
			{
				session->close();
			}
		}
	}

	//解析tcp数据并处理
	static void tcpDataParser(Session_uv* session)
	{
		//选择用哪个数据
		unsigned char* pkgData = (unsigned char*)((session->longPkg != NULL) ? session->longPkg : session->recvBuf);
		while (session->received > 0)
		{
			int pkgSize = 0;
			int headSize = 0;

			//解析头数据并获得长度
			if (!tcpProtocol::readTcpHeader(pkgData, session->received, &pkgSize, &headSize))break;

			//没有接收完数据
			if (session->received < pkgSize)break;

			//收到的数据包比头还小 那么就是不正常的会话  会引起程序挂掉  直接处理掉这个会话
			if (pkgSize <= headSize)session->close();

			unsigned char* rawData = pkgData + headSize;

			RecCommand((Session*)session, rawData, pkgSize - headSize);

			if (session->received > pkgSize)
				memmove(pkgData, pkgData + pkgSize, session->received - pkgSize);

			session->received -= pkgSize;

			//分配的临时储存大数据包的空间要释放掉
			if (session->received == 0 && session->longPkg != NULL)
			{
				free(session->longPkg);
				session->longPkg = NULL;
				session->longPkgSize = 0;
			}


		}
	}

	//解析web数据并处理
	static void webDataParser(Session_uv* session)
	{
		//选择用哪个数据
		unsigned char* pkgData = (unsigned char*)((session->longPkgSize != NULL) ? session->longPkg : session->recvBuf);
		while (session->received > 0)
		{
			int pkgSize = 0;
			int headSize = 0;

			//客户端关闭了页面
			if (pkgData[0] == 0x88)
			{
				session->close();
				break;
			}

			//解析头数据并获得长度
			if (!webProtocol::readWebHeader(pkgData, session->received, &pkgSize, &headSize))break;
	
			//没有接收完数据
			if (session->received < pkgSize)break;

			unsigned char* rawData = pkgData + headSize;
			unsigned char* mask = rawData - 4;

			webProtocol::parserRecvData(rawData, mask, pkgSize - headSize);

			RecCommand((Session*)session,rawData, pkgSize - headSize);

			if (session->received > pkgSize)
				memmove(pkgData, pkgData + pkgSize, session->received - pkgSize);

			session->received -= pkgSize;

			//分配的临时储存大数据包的空间要释放掉
			if (session->received == 0 && session->longPkg != NULL)
			{
				free(session->longPkg);
				session->longPkg = NULL;
				session->longPkgSize = 0;
			}
		

		}
	}

	//udp内存
	static void uv_udpBuffmem(uv_handle_t* handle, size_t sizemem, uv_buf_t* uvBuf)
	{
		//sizemem = sizemem < 8096 ? 8096 : sizemem;
		udpRecvInfo* buf = (udpRecvInfo*)handle->data;
		if (buf->maxRecvLen < sizemem)
		{
			if (buf->recvBuf)
			{
				free(buf->recvBuf);
				buf->recvBuf = NULL;
			}
			buf->recvBuf = (char*)malloc(sizemem);
			buf->maxRecvLen = sizemem;
		}
		uvBuf->base = buf->recvBuf;
		uvBuf->len = sizemem;
	}

	//分配每一个tcpsocket内存
	static void uv_tcpBuffmem(uv_handle_t* handle, size_t sizemem, uv_buf_t* buf)
	{
		Session_uv* session = (Session_uv*)handle->data;
		//如果接收数据的空间足够
		if (session->received < RECVBUF)
			*buf = uv_buf_init(session->recvBuf + session->received, RECVBUF - session->received);
		else
		{
			if (session->longPkg == NULL)
			{
				//web
				if (session->socketType == WEB_SOCKET && session->wsShake)
				{
					int pkgSize = 0;
					int headSize = 0;
					webProtocol::readWebHeader((unsigned char*)session->recvBuf, session->received, &pkgSize, &headSize);

					session->longPkgSize = pkgSize;
					session->longPkg = (char*)malloc(pkgSize);
					memcpy(session->longPkg, session->recvBuf, session->received);
				}
				//tcp
				else
				{
					int pkg_size;
					int head_size;
					tcpProtocol::readTcpHeader((unsigned char*)session->recvBuf, session->received, &pkg_size, &head_size);
					session->longPkgSize = pkg_size;
					session->longPkg = (char*)malloc(pkg_size);
					memcpy(session->longPkg, session->recvBuf, session->received);
				}
			}
		
			*buf = uv_buf_init(session->longPkg+session->received, session->longPkgSize - session->received);
		}
	}

	//读取发过来的数据
	static void uv_readData(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
	{
		Session_uv* session = (Session_uv*)stream->data;
		//小于0表示客户端主动关闭连接
		if (nread < 0)
		{
			session->close();
			return;
		}
		//每一次接收数据都会记录大小
		session->received += nread;
		//web数据
		if (session->socketType == WEB_SOCKET)
		{
			//第一次来要握手
			if (session->wsShake == 0)
			{
				if (webProtocol::webserShakeHand((Session*)session, session->recvBuf, session->received))
				{
					//握手成功
					session->wsShake = 1;
					//接收旗标
					session->received = 0;
				}
			}
			//之后
			else
				webDataParser(session);

		}
		//tcp数据
		else
		{
			tcpDataParser(session);
		}
	}

	//开始监听udpsocket
	static void uv_startUdpEvent(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags)
	{
		Session_uv udpServer;
		udpServer.udpHandler = *handle;
		udpServer.udpStatus = 1;
		udpServer.udpAdddr = addr;
		uv_ip4_name((sockaddr_in*)addr, udpServer.cipAddr, sizeof(udpServer.cipAddr));
		udpServer.cPort = ntohs(((sockaddr_in*)addr)->sin_port);

		RecCommand((Session*)& udpServer,(unsigned char*)buf->base, nread);
	}

	//开始监听tcpsocket
	static void uv_startTcpEvent(uv_stream_t* server, int status)
	{
		Session_uv* session = Session_uv::create();
		uv_tcp_t* client = &session->tcpHandler;
		memset(client, 0, sizeof(uv_tcp_t));
		uv_tcp_init(uv_default_loop(), client);
		client->data = (void*)session;
		uv_accept(server, (uv_stream_t*)client);

		struct sockaddr_in cAddr;
		int len = sizeof(cAddr);
		//得到客户端信息
		uv_tcp_getpeername(client, (sockaddr*)&cAddr, &len);
		uv_ip4_name(&cAddr, (char*)session->cipAddr, 64);

		session->cPort = ntohs(cAddr.sin_port);
		session->socketType = (int)server->data;
		server->data = (void*)session;
		uv_read_start((uv_stream_t*)client, uv_tcpBuffmem, uv_readData);
	}

}

NetBus* NetBus::instance()
{
	return &g_NetBus;
}

struct connect_cb {
	void(*on_connected)(int err, Session* s, void* udata);
	void* udata;
};

static void
after_connect(uv_connect_t* handle, int status) {
	Session_uv* s = (Session_uv*)handle->handle->data;
	struct connect_cb* cb = (struct connect_cb*)handle->data;

	if (status) {
		if (cb->on_connected) {
			cb->on_connected(1, NULL, cb->udata);
		}
		logError("uv_tcp_connect Error!!!");
		s->close();
		free(cb);
		free(handle);
		return;
	}

	if (cb->on_connected) {
		cb->on_connected(0, (Session*)s, cb->udata);
	}
	uv_read_start((uv_stream_t*)handle->handle, uv_tcpBuffmem, uv_readData);

	free(cb);
	free(handle);
}

void NetBus::startWebServer(const char* ip, int port)
{
	uv_tcp_t* listen = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	memset(listen, 0, sizeof(listen));
	uv_tcp_init(uv_default_loop(), listen);
	sockaddr_in addr;
	uv_ip4_addr(ip, port, &addr);
	if (uv_tcp_bind(listen, (const sockaddr*)& addr, 0) != 0)
	{
		logError("Bind WebServer Error !!!");
		free(listen);
		return;
	}
	uv_listen((uv_stream_t*)listen, SOMAXCONN, uv_startTcpEvent);
	listen->data = (void*)WEB_SOCKET;
}

void NetBus::startUdpServer(const char* ip, int port)
{
	uv_udp_t* server = (uv_udp_t*)malloc(sizeof(uv_udp_t));
	memset(server, 0, sizeof(uv_udp_t));

	uv_udp_init(uv_default_loop(), server);

	udpRecvInfo* buf = (udpRecvInfo*)malloc(sizeof(udpRecvInfo));
	memset(buf, 0, sizeof(udpRecvInfo));
	server->data = (udpRecvInfo*)buf;

	sockaddr_in addr;
	uv_ip4_addr(ip, port, &addr);
	uv_udp_bind(server, (sockaddr*)&addr, 0);

	uv_udp_recv_start(server, uv_udpBuffmem, uv_startUdpEvent);
}

void NetBus::tcp_connect(const char* ip, int port, void(*on_connected)(int err, Session* s, void* udata), void* udata)
{
	sockaddr_in bind_addr;
	if (uv_ip4_addr(ip, port, &bind_addr)) {
		logError("Can~t bind address!!")
		return;
	}

	Session_uv* s = Session_uv::create();
	uv_tcp_t* client = &s->tcpHandler;
	memset(client, 0, sizeof(uv_tcp_t));
	uv_tcp_init(uv_default_loop(), client);
	client->data = (void*)s;
	s->as_client = 1;
	s->socketType = TCP_SOCKET;
	strcpy(s->sipAddr, ip);
	s->sPort = port;

	uv_connect_t* connect_req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
	connect_cb* cb = (struct connect_cb*)malloc(sizeof(connect_cb));
	cb->on_connected = on_connected;
	cb->udata = udata;
	connect_req->data = (void*)cb;

	uv_tcp_connect(connect_req, client, (struct sockaddr*)&bind_addr, after_connect);
}

void NetBus::run()
{
	//循环监听有连接的socket
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

void NetBus::startTcpServer(const char* ip, int port)
{
	uv_tcp_t* listen = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	memset(listen, 0, sizeof(uv_tcp_t));
	uv_tcp_init(uv_default_loop(), listen);
	sockaddr_in addr;

	//开始bind启动的要服务器的IP的端口
	uv_ip4_addr(ip, port, &addr);
	if (uv_tcp_bind(listen, (const sockaddr*)& addr, 0) != 0)
	{
		logError("Bind TcpServer Error !!!");
		free(listen);
		return;
	}

	//bind成功设置客户端连接数
	uv_listen((uv_stream_t*)listen, SOMAXCONN, uv_startTcpEvent);
	listen->data = (void*)TCP_SOCKET;
}

void NetBus::init()
{
	ServiceManager::init();
	initSessionAllocer();
}