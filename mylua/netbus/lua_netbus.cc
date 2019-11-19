#include "luainterface/luainterface.h"
#include "lua_netbus.h"
#include "netbus/netbus.h"

int startTcpServer(lua_State* L)
{
	if (lua_gettop(L) != 2)return 0;
	const char* serverIp = lua_tostring(L, 1, 0);
	int port = lua_tointeger(L, 2, 0);
	if (serverIp && port)
		NetBus::instance()->startTcpServer((char*)serverIp,port);
	return 0;
}

int startWebServer(lua_State* L)
{
	if (lua_gettop(L) != 2)return 0;
	const char* serverIp = lua_tostring(L, 1, 0);
	int port = lua_tointeger(L, 2, 0);
	if (serverIp && port)
		NetBus::instance()->startWebServer(serverIp, port);
	return 0;
}

int startUdpServer(lua_State* L)
{
	if (lua_gettop(L) != 2)return 0;
	const char* serverIp = lua_tostring(L, 1, 0);
	int port = lua_tointeger(L, 2, 0);
	if (serverIp && port)
		NetBus::instance()->startUdpServer(serverIp, port);
	return 0;
}

void connectInfo(int err, Session* s, void* udata)
{
	if (err)
	{
		lua_pushinteger(LuaInterface::getGlobalState(), err);
		lua_pushnil(LuaInterface::getGlobalState());
	}
	else
	{
		lua_pushinteger(LuaInterface::getGlobalState(), err);
		tolua_pushuserdata(LuaInterface::getGlobalState(),s);
	}
	LuaInterface::executeLuaFunc((int)udata, 2);
	LuaInterface::removeLuaFunc((int)udata);
}

int getawayConnect(lua_State* L)
{
	const char* ip = luaL_checklstring(L, 1,0);
	int port = luaL_checkinteger(L, 2);
	int handle = toluafix_ref_function(L, 3, 0);
	if (ip && port && handle)
		NetBus::instance()->tcp_connect(ip, port, connectInfo,(void*)handle);
	return 0;
}

int regist_netbus_export(lua_State* L)
{
	//_G是lua中运行的全局表
	lua_getglobal(L, "_G");
	if (lua_istable(L, -1))
	{
		tolua_open(L);
		//表示类
		tolua_module(L, "NetBus", 0);
		tolua_beginmodule(L, "NetBus");
		//表示方法
		tolua_function(L, "startTcpServer", startTcpServer);
		tolua_function(L, "startWebServer", startWebServer);
		tolua_function(L, "startUdpServer", startUdpServer);
		tolua_function(L, "getawayConnect", getawayConnect);

		tolua_endmodule(L);
	}

	lua_pop(L, 1);
	return 0;
}
