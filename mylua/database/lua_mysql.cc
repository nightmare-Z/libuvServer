#include "luainterface/luainterface.h"
#include "database/mysqlmanager.h"
#include "lua_mysql.h"

static void mysqlOperationCon(const char* err, void* context, void* uData)
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
	int port = tolua_tonumber(L, 2, 0);
	char* db = (char*)tolua_tostring(L, 3, 0);
	char* user = (char*)tolua_tostring(L, 4, 0);
	char* pswd = (char*)tolua_tostring(L, 5, 0);
	//函数接收
	int handle = toluafix_ref_function(L, 6, 0);
	if (ip && port && db && user && pswd)
		mysqlManager::connect(ip, port, db, user, pswd, mysqlOperationCon,(void*)handle);
	return 0;
}

static int close(lua_State* L)
{
	void* context = tolua_touserdata(LuaInterface::getGlobalState(), 1, 0);
	if (context) {
		mysqlManager::close(context);
	}
	return 0;
}

static void pushMysqlQueryRes(int num, sql::ResultSet* result, sql::ResultSetMetaData* res)
{
	//第二个子表是获取到的数据
	lua_newtable(LuaInterface::getGlobalState());
	int index = 1;
	sql::SQLString colName;
	sql::SQLString toLua;

	for (int i = 1; i <= num; i++)
	{
		colName = res->getColumnName(i);
		toLua = result->getString(colName.c_str());
		lua_pushstring(LuaInterface::getGlobalState(), toLua.c_str());
		lua_rawseti(LuaInterface::getGlobalState(), -2, index);          /* table[index] = value, L: table */
		++index;
	}
}

static void mysqlOperationQue(const char* err, sql::ResultSet* result,void* uData)
{
	void* context = tolua_touserdata(LuaInterface::getGlobalState(), 1, 0);

	if (err)
	{
		lua_pushstring(LuaInterface::getGlobalState(), err);
		lua_pushnil(LuaInterface::getGlobalState());
	}
	//没错误
	else
	{
		lua_pushnil(LuaInterface::getGlobalState());
		if (result)
		{
			//这个是push的第一个参数err
			lua_pushnil(LuaInterface::getGlobalState());

			//这是第二个参数--queryResul
			int index = 1;
			sql::ResultSetMetaData* res = result->getMetaData();

			//如果有返回结果 则用一个lua表来接收
			lua_newtable(LuaInterface::getGlobalState());

			lua_newtable(LuaInterface::getGlobalState());
			int colInfo = 1;
			for (unsigned int i = 1; i <= res->getColumnCount(); i++)
			{
				lua_pushstring(LuaInterface::getGlobalState(), res->getColumnName(i).c_str());
				lua_rawseti(LuaInterface::getGlobalState(), -2, colInfo);          /* table[index] = value, L: table */
				++colInfo;
			}
			lua_rawseti(LuaInterface::getGlobalState(), -2, index);
			++index;
			while (result->next())
			{
				//打印一列查询到的信息并push
				pushMysqlQueryRes(res->getColumnCount(),result,res);
				lua_rawseti(LuaInterface::getGlobalState(), -2, index);          /* table[index] = value, L: table */
				++index;
			}
		}
		else
		{
			lua_pushstring(LuaInterface::getGlobalState(), "query sqlstatement success");
		}
	}
	LuaInterface::executeLuaFunc((int)uData, 2);
	LuaInterface::removeLuaFunc((int)uData);
}

static int query(lua_State* L)
{
	void* context = tolua_touserdata(L, 1, 0);
	char* sql = (char*)tolua_tostring(L, 2, 0);
	int handler = toluafix_ref_function(L, 3, 0);
	if (context && sql && handler)
		mysqlManager::query(context, sql, mysqlOperationQue, (void*)handler);
	return 0;
}


int regist_mysql_export(lua_State* L)
{
	//_G是lua中运行的全局表
	lua_getglobal(L, "_G");
	if (lua_istable(L, -1))
	{
		tolua_open(L);
		//表示类
		tolua_module(L, "mysqlManager", 0);
		tolua_beginmodule(L, "mysqlManager");
		//表示方法
		tolua_function(L, "connect", connect);
		tolua_function(L, "close", close);
		tolua_function(L, "query", query);

		tolua_endmodule(L);
	}

	lua_pop(L, 1);
	return 0;
}
