#include "luainterface/luainterface.h"
#include "database/redismanager.h"
#include "lua_redis.h"

static void pushredisQueryRes(redisReply* result)
{
	switch (result->type)
	{
	case REDIS_REPLY_INTEGER:
		lua_pushinteger(LuaInterface::getGlobalState(), result->integer);
		break;
	case REDIS_REPLY_STRING:
	case REDIS_REPLY_STATUS:
		lua_pushstring(LuaInterface::getGlobalState(), result->str);
		break;
	case REDIS_REPLY_NIL:
		lua_pushnil(LuaInterface::getGlobalState());
		break;
		//如果是个数组  就递归push  这就是我半路包成函数的原因
	case REDIS_REPLY_ARRAY:
		lua_newtable(LuaInterface::getGlobalState());
		int index = 1;
		for (int i = 0; i < result->elements; i++)
		{
			pushredisQueryRes(result->element[i]);
			lua_rawseti(LuaInterface::getGlobalState(), -2, index);
			++index;
		}
		break;
	}
}

static void redisOperationCon(const char* err, void* context, void* uData)
{
	//有错误
	if (err)
	{
		lua_pushstring(LuaInterface::getGlobalState(), err);
		lua_pushnil(LuaInterface::getGlobalState());
	}
	//没错误
	else
	{
		lua_pushnil(LuaInterface::getGlobalState());
		tolua_pushuserdata(LuaInterface::getGlobalState(), context);
	}
	LuaInterface::executeLuaFunc((int)uData, 2);
	LuaInterface::removeLuaFunc((int)uData);
}

static int connect(lua_State* L)
{
	char* ip = (char*)tolua_tostring(L, 1, 0);
	int port = (int)tolua_tonumber(L, 2, 0);
	//函数接收
	int handle = toluafix_ref_function(L, 3, 0);
	if (ip && port && handle)
		redisManager::connect(ip, port, redisOperationCon, (void*)handle);
	return 0;
}

static int closeRedis(lua_State* L)
{
	void* context = tolua_touserdata(LuaInterface::getGlobalState(), 1, 0);
	if (context) {
		redisManager::closeRedis(context);
	}
	return 0;
}


static void redisOperationQue(const char* err, redisReply* result, void* uData)
{
	//有错误
	if (err)
	{
		lua_pushstring(LuaInterface::getGlobalState(), err);
		lua_pushnil(LuaInterface::getGlobalState());
	}
	//没错误
	else
	{
		lua_pushnil(LuaInterface::getGlobalState());
		//tolua_pushuserdata(LuaInterface::getGlobalState(), result);
		pushredisQueryRes(result);
	}
	LuaInterface::executeLuaFunc((int)uData, 2);
	LuaInterface::removeLuaFunc((int)uData);
}

static int query(lua_State* L)
{
	void* context = tolua_touserdata(L, 1, 0);
	char* cmd = (char*)tolua_tostring(L, 2, 0);
	int handler = toluafix_ref_function(L, 3, 0);
	if (context && cmd)
		redisManager::query(context, cmd, redisOperationQue, (void*)handler);
	return 0;
}


int regist_redis_export(lua_State* L)
{
	//_G是lua中运行的全局表
	lua_getglobal(L, "_G");
	if (lua_istable(L, -1))
	{
		tolua_open(L);
		//表示类
		tolua_module(L, "redisManager", 0);
		tolua_beginmodule(L, "redisManager");
		//表示方法
		tolua_function(L, "connect", connect);
		tolua_function(L, "closeRedis", closeRedis);
		tolua_function(L, "query", query);

		tolua_endmodule(L);
	}

	lua_pop(L, 1);
	return 0;
}
