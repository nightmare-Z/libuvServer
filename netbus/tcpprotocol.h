#ifndef __TCPPROTOCOL_H__
#define __TCPPROTOCOL_H__

class tcpProtocol
{
public:
	//读取协议头数据 
	static bool readTcpHeader(unsigned char* data,int dataLen,int* pkgSize,int* outHeaderSize);
	//解析发过来的数据
	static unsigned char* parserRecvData(const unsigned char* rawData,int len,int* pkgLen);
	//返回对应的操作
	static void freePackageData(unsigned char* tcpPkg);
};

#endif