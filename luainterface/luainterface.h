#ifndef __LUAINTERFACE_H__
#define __LUAINTERFACE_H__

#include "./package/lua/lua.hpp"


#ifdef __cplusplus
extern "C" {
#endif
#include "package/toluapp/tolua++.h"

#ifdef __cplusplus
}
#endif

#include "package/toluapp/tolua_fix.h"

class LuaInterface {
public:

	static lua_State* getGlobalState();

	static void init();

	static void exit();

	static bool exeLuaFile(const char* filename);

	static void registFuncToLua(const char* luaFuncName, int(*cFuncName)(lua_State* L));

	static void addLuaWorkEnv(const char* path);

public:
	
	static int executeLuaFunc(int nHandler, int numArgs);
	static void removeLuaFunc(int nHandler);
};

#endif