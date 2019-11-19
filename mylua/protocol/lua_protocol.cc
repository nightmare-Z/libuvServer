#include "luainterface/luainterface.h"
#include "lua_protocol.h"
#include "netbus/protocolmanager.h"

static int init(lua_State* L)
{
	int type = lua_tointeger(L, 1);
	if (type != PROTOCOL_JSON && type != PROTOCOL_BUF)
		return 0;
	else
		protocolManager::init(type);
	return 0;
}

static int getProtocolType(lua_State* L)
{
	lua_pushinteger(L, protocolManager::getProtocolType());
	return 1;
}

static int registerCmdMap(lua_State* L)
{
	std::map<int, std::string> cmdMap;
	//获取传过来的表的元素个数
	int len = luaL_len(L, 1);
	if (len > 0)
	{
		for (int i = 1; i <= len; i++)
		{
			lua_pushnumber(L, i);
			lua_gettable(L, 1);
			const char* name = luaL_checkstring(L, -1);
			if (name) {
				cmdMap[i] = name;
			}
			lua_pop(L, 1);
		}
		protocolManager::registerCmdMap(cmdMap);
	}
	return 0;
}

int regist_protocol_export(lua_State* L)
{
	//_G是lua中运行的全局表
	lua_getglobal(L, "_G");
	if (lua_istable(L, -1))
	{
		tolua_open(L);
		//表示类
		tolua_module(L, "protocolManager", 0);
		tolua_beginmodule(L, "protocolManager");
		//表示方法
		tolua_function(L, "init", init);
		tolua_function(L, "getProtocolType", getProtocolType);
		tolua_function(L, "registerCmdMap", registerCmdMap);
		tolua_endmodule(L);
	}

	lua_pop(L, 1);
	return 0;
}
