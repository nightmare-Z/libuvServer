#include "netbus/protocolmanager.h"
#include "protocolmap.h"

std::map<int, std::string> protoMap = {
	{0,"userAccountInfo"},
};

void initProtocolCommandMap()
{
	protocolManager::registerCmdMap(protoMap);
}
