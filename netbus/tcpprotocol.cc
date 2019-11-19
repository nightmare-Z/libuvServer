#include <string.h>

#include "tcpprotocol.h"
#include "utils/Cache.h"

extern cacheMan* wbuf_allocer;

bool tcpProtocol::readTcpHeader(unsigned char* data, int dataLen, int* pkgSize, int* outHeaderSize)
{
	if (dataLen < 2)return false;

	*pkgSize = (data[0] | (data[1] << 8));
	*outHeaderSize = 2;

	return true;
}

unsigned char* tcpProtocol::parserRecvData(const unsigned char* rawData, int len, int* pkgLen)
{
	//¼ÓÃÜ
	int headSize = 2;
	*pkgLen = headSize + len;
	unsigned char* dataBuf = (unsigned char*)cacheAllocer(wbuf_allocer ,*pkgLen);
	dataBuf[0] = (unsigned char)((*pkgLen) & 0x000000ff);
	dataBuf[1] = (unsigned char)(((*pkgLen) & 0x0000ff00)>>8);

	memcpy(dataBuf + headSize, rawData, len);


	return dataBuf;
}

void tcpProtocol::freePackageData(unsigned char* tcpPkg)
{
	cacheFree(wbuf_allocer,tcpPkg);
}
