#include "uv.h"
#include "session_uv.h"
#include "webprotocol.h"
#include "tcpprotocol.h"
#include "utils/Cache.h"
#include "servicemanager.h"
#include "utils/logger.h"


//每一个cache分配多少空间
#define CACHE_RECV_SIZE			4096
//每一个session分配多少空间
//#define SESSION_NUM_SIZE		520000
#define SESSION_NUM_SIZE		6000
//每一个tcp命令分配多少空间
#define CACHE_SEND_SIZE			1024
#define CACHE_CMD_SIZE			1024


//全局总cache总空间
struct cacheMan* session_allocer = NULL;

//接收超大数据包时用到的临时内存
static cacheMan* wr_allocer = NULL;

//发数据包给客户端用的内存
cacheMan* wbuf_allocer = NULL;

void initSessionAllocer()
{
	if (session_allocer == NULL)
		session_allocer = cacheCreator(SESSION_NUM_SIZE, sizeof(Session_uv));

	if(wr_allocer == NULL)
		wr_allocer = cacheCreator(CACHE_RECV_SIZE, sizeof(uv_write_t));

	if (wbuf_allocer == NULL)
		wbuf_allocer = cacheCreator(CACHE_SEND_SIZE, CACHE_CMD_SIZE);
}

extern "C"
{
	static void uv_sendUdpBuf(uv_udp_send_t* req, int statu)
	{
		if (statu == 0)
		{

		}
		cacheFree(wr_allocer, req);
	}
	//发送给客户端的数据
	static void uv_sendTcpBuf(uv_write_t* req, int status)
	{
		if (status == 0)
		{

		}
		//发完数据就cache这个临时内存
		cacheFree(wr_allocer, req);
	}
	//关闭客户端时间处理在这里
	static void uv_closeClient(uv_handle_t* handle)
	{
		Session_uv* session = (Session_uv*)handle->data;
		Session_uv::destroy(session);
	}
	//关闭客户端连接
	static void uv_close_oneClient(uv_shutdown_t* shotdown, int status)
	{
		uv_close((uv_handle_t*)shotdown->handle, uv_closeClient);
	}

}

Session_uv* Session_uv::create() {
	//一个链接进来 那么就给他一个cache空间
	Session_uv* s_uv = new Session_uv();
	//s_uv->Session_uv::Session_uv();
	s_uv->init();
	return s_uv;
}

void Session_uv::destroy(Session_uv* s)
{
	s->exit();
	delete s;
	//s->Session_uv::~Session_uv();
	//一个链接销毁 那么就给他回收这个cache空间
	//cacheFree(session_allocer, s);
}

void* Session_uv::operator new(size_t size)
{
	return cacheAllocer(session_allocer, sizeof(Session_uv));
}

void Session_uv::operator delete(void* mem)
{
	cacheFree(session_allocer, mem);
}

void Session_uv::init()
{
	
	this->cPort = 0;
	this->received = 0;
	this->isShotdown = false;
	this->wsShake = 0;
	this->longPkg = NULL;
	this->longPkgSize = 0;
}

void Session_uv::exit()
{
}

void Session_uv::close()
{
	if (this->isShotdown)return;
	//清理服务
	ServiceManager::sessionDisconnect(this);
	//改变自身状态
	this->isShotdown = true;
	//调用netbus关闭  req是请求
	uv_shutdown_t* req = &this->uvShotdown;
	memset(req, 0, sizeof(uv_shutdown_t));
	//如果没有链接到的会话是不能走到shoudown里面的要根据返回值进行自主关闭
	if (uv_shutdown(req, (uv_stream_t*)& this->tcpHandler, uv_close_oneClient) != 0)
	{
		uv_close((uv_handle_t*)& this->tcpHandler, uv_closeClient);
	}
}

void Session_uv::sendData(unsigned char* body, int len)
{
	//tcp  web
	uv_write_t* w_req = (uv_write_t*)cacheAllocer(wr_allocer, sizeof(uv_write_t));
	uv_buf_t w_buf;

	//udp
	if (this->udpStatus==1)
	{
		uv_udp_send_t* w_req = (uv_udp_send_t*)cacheAllocer(wr_allocer, sizeof(uv_udp_send_t));
		uv_buf_t w_buf = uv_buf_init((char*)body, len);

		uv_udp_send(w_req, &this->udpHandler, &w_buf,1,this->udpAdddr, uv_sendUdpBuf);
		return;
	}

	if (this->socketType == WEB_SOCKET)
	{
		if (this->wsShake)
		{
			int webDataLen = 0;
			unsigned char* webPkg = webProtocol::sendPackageData(body, len, &webDataLen);
			w_buf = uv_buf_init((char*)webPkg, webDataLen);
			uv_write(w_req, (uv_stream_s*)& this->tcpHandler, &w_buf, 1, uv_sendTcpBuf);
			webProtocol::freePackageData(webPkg);
		}
		else
		{
			w_buf = uv_buf_init((char*)body, len);
			uv_write(w_req, (uv_stream_t*)& this->tcpHandler, &w_buf, 1, uv_sendTcpBuf);
		}
	}
	else		//protobuf
	{
		int tcpDataLen;
		unsigned char* tcpPkg = tcpProtocol::parserRecvData(body, len, &tcpDataLen);
		w_buf = uv_buf_init((char*)tcpPkg, tcpDataLen);
		uv_write(w_req, (uv_stream_t*)& this->tcpHandler, &w_buf, 1, uv_sendTcpBuf);
		tcpProtocol::freePackageData(tcpPkg);
	}
}

const char* Session_uv::getAddress(int* clientPort)
{
	*clientPort = this->cPort;
	return this->cipAddr; 
}

void Session_uv::sendMessage(commandMessage* msg)
{
	unsigned char* encodePkg = NULL;
	int encodeLen = 0;
	encodePkg = protocolManager::encodeCmdMsg(msg, &encodeLen);
	if (encodePkg)
	{
		this->sendData(encodePkg, encodeLen);

		protocolManager::freeEncodeCmdMsg(encodePkg);
	}
}

void Session_uv::sendMessage(Gateway2Server* cmd)
{
	this->sendData(cmd->rawCmd, cmd->rawLen);
}
