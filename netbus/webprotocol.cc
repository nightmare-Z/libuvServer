#include <stdio.h>
#include <string.h>

#include "utils/Cache.h"
#include "..//package/http_parser/http_parser.h"
#include "..//package/encryption/sha1.h"
#include "..//package/encryption/md5.h"
#include "..//package/encryption/base64_decoder.h"
#include "..//package/encryption/base64_encoder.h"

#include "session.h"
#include "webprotocol.h"
#include "utils/logger.h"

#define SHA1DIGESTSIZE 2048

extern struct cacheMan* wbuf_allocer;

static char filedSecKey[512];
static char valueSecKey[512];
static int isSecKey = 0;
static int hasSecKey = 0;
static int isShakeEnd = 0;  //是否握了手

//密钥
const char* webMagic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

//接收报文的格式
const char* webAccept = "HTTP/1.1 101 Switching Protocols\r\n"
"Upgrade:websocket\r\n"
"Connection: Upgrade\r\n"
"Sec-WebSocket-Accept:%s\r\n"
"WebSocket-Protocol:chat\r\n\r\n";


extern "C"
{
	//接收报文头
	static int onWebHeaderFiled(http_parser* p, const char* at, size_t length)
	{
		if (strncmp(at, "Sec-WebSocket-Key", length) == 0)
			isSecKey = 1;
		else
			isSecKey = 0;

		return 0;
	}

	static int onWebHeaderValue(http_parser* p, const char* at, size_t length)
	{
		if (!isSecKey)return 0;
		strncpy(valueSecKey, at, length);
		valueSecKey[length] = 0;
		hasSecKey = 1;
		return 0;
	}

	static int onWebMessageEnd(http_parser* p)
	{
		isShakeEnd = 1;
		return 0;
	}
}

bool webProtocol::webserShakeHand(Session* session, char* body, int len)
{
	http_parser_settings settings;
	http_parser_settings_init(&settings);
	settings.on_header_field = onWebHeaderFiled;
	settings.on_header_value = onWebHeaderValue;

	//接收报文信息
	settings.on_message_complete = onWebMessageEnd;
	http_parser p;
	http_parser_init(&p, HTTP_REQUEST);
	isSecKey = 0;
	hasSecKey = 0;
	isShakeEnd = 0;
	//拿取接收的报文
	http_parser_execute(&p, &settings, body, len);

	//所有信息接收完毕开始处理
	if (hasSecKey&&isShakeEnd)
	{
		//key     &&&   migic
		static char keyMagic[512];
		static char sha1_key_magic[SHA1DIGESTSIZE];
		static char sendClient[512];

		//加密
		int sha1Size;
		sprintf(keyMagic, "%s%s", valueSecKey, webMagic);
		crypt_sha1((unsigned char*)keyMagic, strlen(keyMagic), (unsigned char*)& sha1_key_magic,&sha1Size);
		int base64Len;
		char* baseBuf = base64_encode((unsigned char*)sha1_key_magic, sha1Size, &base64Len);
		sprintf(sendClient, webAccept, baseBuf);
		base64_encode_free(baseBuf);

		session->sendData((uint8_t*)sendClient, strlen(sendClient));
		return true;
	}
	return false;
}

bool webProtocol::readWebHeader(unsigned char* pkgData, int pkgLen, int* pkgSize, int* outHeaderSize)
{
	if (pkgData[0] != 0x81 && pkgData[0] != 0x82)return false;

	if (pkgLen < 2)return false;

	unsigned int dataLen = pkgData[1] & 0x0000007f;

	int headSize = 2;
	if (dataLen == 126)
	{
		headSize += 2;
		if (pkgLen < headSize)return false;
		dataLen = pkgData[3] | (pkgData[2] << 8);
		
	}
	else if(dataLen == 127)
	{
		headSize += 8;
		if (pkgLen < headSize)return false;
		unsigned int low = pkgData[5] | (pkgData[4] << 8) | (pkgData[3] << 16) | (pkgData[2] << 24);
		unsigned int hight = pkgData[9] | (pkgData[8] << 8) | (pkgData[7] << 16) | (pkgData[6] << 24);
		dataLen = low;
		
	}
	//加4个mask
	headSize += 4;
	*pkgSize = dataLen + headSize;
	*outHeaderSize = headSize;
	return true;
}

void webProtocol::parserRecvData(unsigned char* rawData, unsigned char* mask, int rawLen)
{
	for (int i = 0; i < rawLen; i++)
		rawData[i] = rawData[i] ^ mask[i % 4];
}

unsigned char* webProtocol::sendPackageData(const unsigned char* raw_data, int len, int* webDataLen)
{
	//加密
	int headSize = 2;
	if (len > 0x7d && len < 0x10000)
		headSize += 2;
	else if (len >= 0x10000)
	{
		headSize += 8;
		return nullptr;
	}

	unsigned char* dataBuf = (unsigned char*)cacheAllocer(wbuf_allocer,headSize + len);
	dataBuf[0] = 0x82; //0x81协议是字符串  0x82 是array
	if (len <= 0x7d)
		dataBuf[1] = len;
	else if (len > 0x7d && len < 0x10000)
	{
		dataBuf[1] = 0x7e;
		dataBuf[2] = (len & 0xff00) >> 8;
		dataBuf[3] = len & 0xff;
	}

	memcpy(dataBuf + headSize, raw_data, len);
	*webDataLen = headSize + len;

	return dataBuf;
}

void webProtocol::freePackageData(unsigned char* ws_pkg)
{
	cacheFree(wbuf_allocer,ws_pkg);
}
