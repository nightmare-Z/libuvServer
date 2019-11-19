#include "luainterface/luainterface.h"
#include "lua_logger.h"
#include <string>

//打印lua栈的信息 用于调试出有问题lua文件的信息
void logluamessage(void(*log)(const char* fileName, int line, const char* msg), const char* msg)
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

void luaDebug(const char* fileName, int line, const char* msg)
{
	Logger::log(fileName, line, LOGDEBUG, msg);
}

void luaWarning(const char* fileName, int line, const char* msg)
{
	Logger::log(fileName, line, LOGWARNING, msg);
}

void luaError(const char* fileName, int line, const char* msg)
{
	Logger::log(fileName, line, LOGERROR, msg);
}

int loggerDebug(lua_State* L)
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
	logluamessage(luaDebug, t.c_str());
	return 0;
}

int loggerWarning(lua_State* L)
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
	logluamessage(luaWarning, t.c_str());
	return 0;
}

int loggerError(lua_State* L)
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
	logluamessage(luaError, t.c_str());
	return 0;
}

static int
init(lua_State* L) {
	const char* path = lua_tostring(L, 1);
	const char* prefix = lua_tostring(L, 2);
	bool std_out = lua_toboolean(L, 3);
	if (path && prefix || std_out)
		Logger::init(path, prefix, std_out);

	return 0;
}

int regist_logger_export(lua_State* L)
{
	//_G是lua中运行的全局表
	lua_getglobal(L, "_G");
	if (lua_istable(L, -1))
	{
		tolua_open(L);
		//表示类
		tolua_module(L, "Logger", 0);
		tolua_beginmodule(L, "Logger");
		//表示方法
		tolua_function(L, "init", init);
		tolua_function(L, "d", loggerDebug);
		tolua_function(L, "w", loggerWarning);
		tolua_function(L, "e", loggerError);
		tolua_endmodule(L);
	}
	lua_pop(L, 1);
	return 0;
}
