#define MAIN_LUA

#ifdef MAIN_CPP
#include "netbus/netbus.h"
#include "netbus/servicemanager.h"
#include "netbus/protocolmanager.h"
#include "database/mysqlmanager.h"
#include "database/redismanager.h"
#include "utils/logger.h"
#include "main/protocol/protocolmap.h"

int main(int argc, char** argv)
{
	protocolManager::init(PROTOCOL_BUF);
	Logger::init((char*)"log/main", (char*)"main.cpp", true);
	initProtocolCommandMap();
	NetBus::instance()->init();
	NetBus::instance()->startTcpServer("127.0.0.1", 1270);
	NetBus::instance()->startWebServer("127.0.0.1", 97);
	NetBus::instance()->startUdpServer("127.0.0.1", 5796);
	NetBus::instance()->run();
	return 0;
}

#endif // MAIN_CPP

#ifdef MAIN_LUA
#include "netbus/netbus.h"
#include "luainterface/luainterface.h"
#include <stdlib.h>
#include <string>
int main(int argc, char** argv)
{
	NetBus::instance()->init();
	if (argc != 3)
	{
#if 0
		LuaInterface::init();
		LuaInterface::addLuaWorkEnv("../../");
		LuaInterface::exeLuaFile("main.lua");
#else
		LuaInterface::init();
		LuaInterface::addLuaWorkEnv("../main/lua/");
		LuaInterface::exeLuaFile("../main/lua/serviceServer/gateway/main.lua");
#endif
	}
	else
	{
		LuaInterface::init();
		std::string env = argv[1];
		if (*(env.end() - 1) != '/')
			env += "/";
		LuaInterface::addLuaWorkEnv(env.c_str());
		std::string fileName = env + argv[2];
		LuaInterface::exeLuaFile(fileName.c_str());
	}
	NetBus::instance()->run();
	LuaInterface::exit();
	system("pause");
	return 0;
}

#endif // MAIN_LUA