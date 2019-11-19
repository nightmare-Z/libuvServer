#include "luainterface.h"
#include "mylua/utils/lua_logger.h"
#include "mylua/database/lua_mysql.h"
#include "mylua/database/lua_redis.h"
#include "mylua/service/lua_service.h"
#include "mylua/session/lua_session.h"
#include "mylua/utils/lua_schedule.h"
#include "mylua/netbus/lua_netbus.h"
#include "mylua/protocol/lua_protocol.h"

#include <string>

//全局的lua虚拟机接口
lua_State* gLuaState = NULL;

//打印lua栈的信息 用于调试出有问题lua文件的信息
void logLuaErrorMessage(void(*log)(const char* fileName, int line, const char* msg), const char* msg)
{
	lua_Debug info;
	int depth = 0;
	while (lua_getstack(LuaInterface::getGlobalState(), depth, &info))
	{
		lua_getinfo(LuaInterface::getGlobalState(), "S", &info);
		lua_getinfo(LuaInterface::getGlobalState(), "n", &info);
		lua_getinfo(LuaInterface::getGlobalState(), "l", &info);

		if (info.source[0] == '@')
		{
			log(&info.source[1], info.currentline, msg);
			return;
		}
		++depth;
	}
	if (depth == 0)
	{
		log("trunk", 0, msg);
	}
}

void ErrorPrint(const char* fileName, int line, const char* msg)
{
	Logger::log(fileName, line, LOGERROR, msg);
}

int lua_Error(lua_State* L)
{
	int nargs = lua_gettop(L);
	std::string t;
	for (int i = 1; i <= nargs; i++)
	{
		if (lua_istable(L, i))
			t += "table";
		else if (lua_isnone(L, i))
			t += "none";
		else if (lua_isnil(L, i))
			t += "nil";
		else if (lua_isboolean(L, i))
		{
			if (lua_toboolean(L, i) != 0)
				t += "true";
			else
				t += "false";
		}
		else if (lua_isfunction(L, i))
			t += "function";
		else if (lua_islightuserdata(L, i))
			t += "lightuserdata";
		else if (lua_isthread(L, i))
			t += "thread";
		else
		{
			const char* str = lua_tostring(L, i);
			if (str)
				t += lua_tostring(L, i);
			else
				t += lua_typename(L, lua_type(L, i));
		}
		if (i != nargs)
			t += "\t";
	}
	logLuaErrorMessage(ErrorPrint, t.c_str());
	return 0;
}

//在全局表中找这个nhandle这个id对应的函数
static bool findLuaFuncByHandle(int nHandle)
{
	toluafix_get_function_by_refid(gLuaState, nHandle);
	if (!lua_isfunction(gLuaState, -1))
	{
		logError("[LUA ERROR] function refid '%d' does not reference a Lua function", nHandle);
		lua_pop(gLuaState, 1);
		return false;
	}
	return true;
}

static int exeLuaFuncByHandle(int numArgs)
{
	int functionIndex = -(numArgs + 1);
	if (!lua_isfunction(gLuaState, functionIndex))
	{
		logError("value at stack [%d] is not function", functionIndex);
		lua_pop(gLuaState, numArgs + 1); // remove function and arguments
		return 0;
	}

	int traceback = 0;
	lua_getglobal(gLuaState, "__G__TRACKBACK__");                         /* L: ... func arg1 arg2 ... G */
	if (!lua_isfunction(gLuaState, -1))
	{
		lua_pop(gLuaState, 1);                                            /* L: ... func arg1 arg2 ... */
	}
	else
	{
		lua_insert(gLuaState, functionIndex - 1);                         /* L: ... G func arg1 arg2 ... */
		traceback = functionIndex - 1;
	}

	int error = 0;
	error = lua_pcall(gLuaState, numArgs, 1, traceback);                  /* L: ... [G] ret */
	if (error)
	{
		if (traceback == 0)
		{
			logError("[LUA ERROR] %s", lua_tostring(gLuaState, -1));        /* L: ... error */
			lua_pop(gLuaState, 1); // remove error message from stack
		}
		else                                                            /* L: ... G error */
		{
			lua_pop(gLuaState, 2); // remove __G__TRACKBACK__ and error message from stack
		}
		return 0;
	}

	// get return value
	int ret = 0;
	if (lua_isnumber(gLuaState, -1))
	{
		ret = (int)lua_tointeger(gLuaState, -1);
	}
	else if (lua_isboolean(gLuaState, -1))
	{
		ret = (int)lua_toboolean(gLuaState, -1);
	}
	// remove return value from stack
	lua_pop(gLuaState, 1);                                                /* L: ... [G] */

	if (traceback)
	{
		lua_pop(gLuaState, 1); // remove __G__TRACKBACK__ from stack      /* L: ... */
	}

	return ret;
}

lua_State* LuaInterface::getGlobalState()
{
	return gLuaState;
}

//规避lua出错中断程序的出错处理
static int lua_panic(lua_State* L)
{
	const char* msg = luaL_checkstring(L, -1);
	if (msg)
	{
		logLuaErrorMessage(ErrorPrint, msg);
	}
	return 0;
}

int lua_findMyselfWorkEnv(lua_State* L)
{
	const char* path = lua_tostring(L, 1);
	LuaInterface::addLuaWorkEnv(path);
	return 0;
}

//初始化 并链接c/c++ 函数
void LuaInterface::init()
{
	gLuaState = luaL_newstate();
	//防止lua语法崩溃结束程序
	lua_atpanic(gLuaState, lua_panic);
	luaL_openlibs(gLuaState);
	toluafix_open(gLuaState);
	LuaInterface::registFuncToLua("findWorkEnv", lua_findMyselfWorkEnv);
	regist_logger_export(gLuaState);
	regist_mysql_export(gLuaState);
	regist_redis_export(gLuaState);
	regist_service_export(gLuaState);
	regist_session_export(gLuaState);
	regist_schedule_export(gLuaState);
	regist_netbus_export(gLuaState);
	regist_protocol_export(gLuaState);
	regist_releaseRaw_export(gLuaState);
}

void LuaInterface::exit()
{
	if (gLuaState)
	{
		lua_close(gLuaState);
		gLuaState = NULL;
	}
}

bool LuaInterface::exeLuaFile(const char* filename)
{
	if (luaL_dofile(gLuaState, filename))
	{
		lua_Error(gLuaState);
		return false;
	}
	return true;
}

void LuaInterface::registFuncToLua(const char* luaFuncName, int(*cFuncName)(lua_State* L))
{
	lua_pushcfunction(gLuaState, cFuncName);
	lua_setglobal(gLuaState, luaFuncName);
}

void LuaInterface::addLuaWorkEnv(const char* path)
{
	char strPath[1024] = { 0 };
	sprintf(strPath, "local path = string.match([[%s]],[[(.*)/[^/]*$]])\n package.path = package.path .. [[;]] .. path .. [[/?.lua;]] .. path .. [[/?/init.lua]]\n", path);
	luaL_dostring(gLuaState, strPath);
}

int LuaInterface::executeLuaFunc(int nHandler, int numArgs)
{
	int ret = 0;
	if (findLuaFuncByHandle(nHandler))                                /* L: ... arg1 arg2 ... func */
	{
		if (numArgs > 0)
		{
			lua_insert(gLuaState, -(numArgs + 1));                        /* L: ... func arg1 arg2 ... */
		}
		ret = exeLuaFuncByHandle(numArgs);
	}
	lua_settop(gLuaState, 0);
	return ret;
}

void LuaInterface::removeLuaFunc(int nHandler)
{
	toluafix_remove_function_by_refid(gLuaState, nHandler);
}