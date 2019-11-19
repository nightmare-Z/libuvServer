#ifndef __WEBPROTOCOL_H__
#define __WEBPROTOCOL_H__

class webProtocol
{
public:
	//与客户端握手
	static bool webserShakeHand(Session* s, char* body, int len);
	//读取协议头 分出head数据和body数据
	static bool readWebHeader(unsigned char* pkgData, int pkgLen, int* pkgSize,int* outHeaderSize);
	//还原加密的http报表数据
	static void parserRecvData(unsigned char* rawData, unsigned char* mask,int rawLen);
	//发送给客户端的数据包
	static unsigned char* sendPackageData(const unsigned char* raw_data, int len,int* webDataLen);
	//清除已发送的包的数据
	static void freePackageData(unsigned char* ws_pkg);
};

#endif