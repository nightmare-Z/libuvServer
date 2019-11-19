#include "utils/logger.h"
#include "netbus/protocolmanager.h"
#include "utils/Cache.h"

#define COMMAND_HEAD			8

//全局协议类型
static int gProtocolType = PROTOCOL_BUF;
//proto映射表
static std::map<int, std::string> gProtoMap;

//创建一个协议消息
Message* protocolManager::createMessage(const char* nameType)
{
	Message* message = NULL;

	//快速查找命令
	const Descriptor* descriptor =
		DescriptorPool::generated_pool()->FindMessageTypeByName(nameType);
	if (descriptor)
	{
		const Message* protocolType =
			MessageFactory::generated_factory()->GetPrototype(descriptor);

		if (protocolType)
			message = protocolType->New();
	}
	return message;
}

//删掉协议消息
void protocolManager::releaseMessage(Message* message)
{
	delete message;
}


void protocolManager::init(int protocolType)
{
	gProtocolType = protocolType;
}

int protocolManager::getProtocolType()
{
	return gProtocolType;
}

const char* protocolManager::getProtoCurrentMapName(int commandType)
{
	return gProtoMap[commandType].c_str();
}

void protocolManager::registerCmdMap(std::map<int, std::string>& protoMap)
{
	std::map<int, std::string>::iterator it;
	for (it = protoMap.begin(); it != protoMap.end(); it++)
	{
		gProtoMap[it->first] = it->second;
	}
}

bool protocolManager::decodeCmdMsg(unsigned char* cmd, int cmdLen, commandMessage** outMessage)
{
	*outMessage = NULL;

	if (cmdLen < COMMAND_HEAD)return false;

	commandMessage* msg = (commandMessage*)malloc(sizeof(commandMessage));

	msg->serverType = cmd[0] | (cmd[1] << 8);
	msg->commandType = cmd[2] | (cmd[3] << 8);
	msg->tags = cmd[4] | (cmd[5] << 8) | (cmd[6] << 16) | (cmd[7] << 24);
	msg->body = NULL;
	*outMessage = msg;
	if (cmdLen == COMMAND_HEAD)return true;

	if (gProtocolType == PROTOCOL_JSON)
	{
		int jsonLen = cmdLen - COMMAND_HEAD;
		char* jsonStr = (char*)malloc(jsonLen + 1);
		memcpy(jsonStr, cmd + COMMAND_HEAD, jsonLen);
		jsonStr[jsonLen] = 0;
		msg->body = (void*)jsonStr;
	}
	else
	{		
		Message* pM = createMessage(gProtoMap[msg->commandType].c_str());
		if (pM == NULL)
		{
			logError("ProtoBuf : not have this command !!!");
			free(msg);
			*outMessage = NULL;
			return false;
		}
		if (!pM->ParseFromArray(cmd+ COMMAND_HEAD,cmdLen- COMMAND_HEAD))
		{
			free(msg);
			*outMessage = NULL;

			releaseMessage(pM);
			return false;
		}
		msg->body = pM;
		

	}
	return true;
}

void protocolManager::freeDecodeCmdMsg(commandMessage* cmdmsg)
{
	if (cmdmsg->body)
	{
		if (gProtocolType == PROTOCOL_JSON)
		{
			free(cmdmsg->body);
			cmdmsg->body = NULL;
		}
		else
		{
			Message* pM = (Message*)cmdmsg->body;
			delete pM;
			cmdmsg->body = NULL;
		}
	}

	free(cmdmsg);
}

unsigned char* protocolManager::encodeCmdMsg(const commandMessage* cmdmsg, int* outLen)
{
	int rawLen = 0;
	unsigned char* rawData = NULL;
	*outLen = 0;
	if (gProtocolType == PROTOCOL_JSON)
	{
		char* json_str = NULL;
		int len = 0;
		if (cmdmsg->body) {
			json_str = (char*)cmdmsg->body;
			len = strlen(json_str);
		}
		rawData = (unsigned char*)malloc(COMMAND_HEAD + len);
		if (cmdmsg->body != NULL) {
			memcpy(rawData + COMMAND_HEAD, json_str, len - 1);
			rawData[COMMAND_HEAD + len] = 0;
		}

		*outLen = (len + COMMAND_HEAD);
	}
	else if(gProtocolType == PROTOCOL_BUF)
	{
		google::protobuf::Message* p_m = NULL;
		int pf_len = 0;
		if (cmdmsg->body) {
			p_m = (google::protobuf::Message*)cmdmsg->body;
			pf_len = p_m->ByteSize();
		}
		rawData = (unsigned char*)malloc(COMMAND_HEAD + pf_len);

		if (cmdmsg->body) {
			if (!p_m->SerializePartialToArray(rawData + COMMAND_HEAD, pf_len)) {
				free(rawData);
				return NULL;
			}
		}
		*outLen = (pf_len + COMMAND_HEAD);
	}

	rawData[0] = (cmdmsg->serverType & 0x000000ff);
	rawData[1] = ((cmdmsg->serverType & 0x0000ff00)>>8);
	rawData[2] = (cmdmsg->commandType & 0x000000ff);
	rawData[3] = ((cmdmsg->commandType & 0x0000ff00) >> 8);

	memcpy(rawData + 4, &cmdmsg->tags, 4);

	return rawData;
}

void protocolManager::freeEncodeCmdMsg(unsigned char* raw)
{
	free( raw);
}

bool protocolManager::decodeMsgHead(unsigned char* cmd, int cmdLen, Gateway2Server* outRaw)
{
	if (cmdLen < COMMAND_HEAD)return false;
	outRaw->serverType = cmd[0] | (cmd[1] << 8);
	outRaw->commandType = cmd[2] | (cmd[3] << 8);
	outRaw->tags = cmd[4] | (cmd[5] << 8) | (cmd[6] << 16) | (cmd[7] << 24);

	outRaw->rawCmd = cmd;
	outRaw->rawLen = cmdLen;
	return true;
}
