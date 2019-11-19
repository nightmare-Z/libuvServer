#ifndef __PROTOCOLMANAGER_H__
#define __PROTOCOLMANAGER_H__
#include "google/protobuf/message.h"
#include <string>
#include <map>

using namespace google::protobuf;
//协议分类
enum
{
	PROTOCOL_JSON = 0,
	PROTOCOL_BUF = 1,
};

//客户端命令
struct commandMessage 
{
	//服务号
	int serverType; 
	//命令号
	int commandType;
	//用户
	unsigned int tags;
	//命令
	void* body;
};

struct Gateway2Server
{
	//服务号
	int serverType;
	//命令号
	int commandType;
	//用户
	unsigned int tags;

	unsigned char* rawCmd;

	int rawLen;
};

class protocolManager
{
public:
	//初始化协议消息
	static void init(int protocolType);
	//查看协议类型
	static int getProtocolType();
	//得到当前服务表的名字
	static const char* getProtoCurrentMapName(int commandType);
	//创建一个协议消息
	static Message* createMessage(const char* nameType);
	//删掉协议消息
	static void releaseMessage(Message* message);
	//注册协议映射表
	static void registerCmdMap(std::map<int,std::string>& protoMap);
	//加密要发送的数据
	static bool decodeCmdMsg(unsigned char* cmd,int cmdLen,struct commandMessage** outMessage);
	static void freeDecodeCmdMsg(struct commandMessage* cmdmsg);
	//解密发过来的数据
	static unsigned char* encodeCmdMsg(const struct commandMessage* cmdmsg,int* outLen);
	static void freeEncodeCmdMsg(unsigned char* raw);
	//网关解密头来转发服务
	static bool decodeMsgHead(unsigned char* cmd, int cmdLen, struct Gateway2Server* outRaw);
};

#endif