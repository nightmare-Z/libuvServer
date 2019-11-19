#include "luainterface/luainterface.h"
#include "lua_schedule.h"
#include "utils/time.h"
#include "utils/Cache.h"

struct scheduleInfo
{
	//保存的回调函数handle
	int handle;
	//次数
	int repeatCount;
};

void luaScheduler(void* uData)
{
	scheduleInfo* tr = (scheduleInfo*)uData;
	LuaInterface::executeLuaFunc(tr->handle, 0);
	if (tr->repeatCount == -1) {
		return;
	}

	tr->repeatCount--;
	if (tr->repeatCount <= 0) {
		LuaInterface::removeLuaFunc(tr->handle);
		smallCacheFree(tr);
	}
}

static int lua_scheduleRepeat(lua_State* L)
{
	//回调函数标识
	int handle = toluafix_ref_function(L, 1, 0);
	//多少ms后开始启动
	int afterMsec = lua_tointeger(L, 2);
	//启动多少次
	int repeatCount = lua_tointeger(L, 3);
	//每次启动多少秒
	int repeatMsec = lua_tointeger(L, 4);

	if (handle && afterMsec && repeatCount)
	{
		if (!repeatMsec)
			repeatMsec = afterMsec;
		scheduleInfo* ts = (scheduleInfo*)smallCacheAllocer(sizeof(scheduleInfo));
		ts->handle = handle;
		ts->repeatCount = repeatCount;
		tolua_pushuserdata(L, scheduleRepeat(luaScheduler, ts, afterMsec, ts->repeatCount, repeatMsec));
		return 1;
	}
	if (handle != 0)
		LuaInterface::removeLuaFunc(handle);
	lua_pushnil(L);

	return 1;
}

static int lua_cancelTimer(lua_State* L)
{
	if (lua_isuserdata(L, 1))
	{
		timer* scheduler = (timer*)lua_touserdata(L, 1);
		scheduleInfo* tr = (scheduleInfo*)get_timer_udata(scheduler);
		LuaInterface::removeLuaFunc(tr->handle);
		smallCacheFree(tr);
		cancelTimer(scheduler);
	}
	return 0;
}

static int lua_scheduleOnce(lua_State* L)
{
	int handle = toluafix_ref_function(L, 1, 0);
	int afterMsec = lua_tointeger(L, 2);

	if (handle && afterMsec)
	{
		if (afterMsec > 0)
		{
			scheduleInfo* tr = (scheduleInfo*)smallCacheAllocer(sizeof(scheduleInfo));
			tr->handle = handle;
			tr->repeatCount = 1;
			struct timer* t = scheduleOnce(luaScheduler, (void*)tr, afterMsec);
			tolua_pushuserdata(L, t);
			return 1;
		}
		if (handle != 0)
			LuaInterface::removeLuaFunc(handle);
		lua_pushnil(L);

		return 1;
	}
	if (handle != 0)
		LuaInterface::removeLuaFunc(handle);
	lua_pushnil(L);

	return 1;
}

int regist_schedule_export(lua_State* L)
{
	//_G是lua中运行的全局表
	lua_getglobal(L, "_G");
	if (lua_istable(L, -1))
	{
		tolua_open(L);
		//表示类
		tolua_module(L, "scheduler", 0);
		tolua_beginmodule(L, "scheduler");
		//表示方法
		tolua_function(L, "scheduleRepeat", lua_scheduleRepeat);
		tolua_function(L, "cancelTimer", lua_cancelTimer);
		tolua_function(L, "scheduleOnce", lua_scheduleOnce);
		tolua_endmodule(L);
	}
	lua_pop(L, 1);
	return 0;
}