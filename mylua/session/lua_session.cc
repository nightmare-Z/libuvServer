#include "luainterface/luainterface.h"
#include "netbus/session.h"
#include "utils/logger.h"
#include "lua_session.h"

static Message* luaTableToProtobuf(lua_State* L, int stack, const char* name)
{
	if (lua_istable(L, stack))
	{
		Message* message = protocolManager::createMessage(name);
		if (message)
		{
			//获取每个字段的描述
			const Reflection* reflection = message->GetReflection();
			const Descriptor* descriptor = message->GetDescriptor();

			// 遍历table的所有key， 并且与 protobuf结构相比较。如果require的字段没有赋值， 报错！ 如果找不到字段，报错！
			for (int32_t index = 0; index < descriptor->field_count(); ++index) {
				const FieldDescriptor* fd = descriptor->field(index);
				const string& name = fd->name();

				bool isRequired = fd->is_required();
				bool bReapeted = fd->is_repeated();
				lua_pushstring(L, name.c_str());
				lua_rawget(L, stack);

				bool isNil = lua_isnil(L, -1);

				if (bReapeted) {
					if (isNil) {
						lua_pop(L, 1);
						continue;
					}
					else {
						bool isTable = lua_istable(L, -1);
						if (!isTable) {
							logError("cant find required repeated field %s\n", name.c_str());
							protocolManager::releaseMessage(message);
							return NULL;
						}
					}

					lua_pushnil(L);
					for (; lua_next(L, -2) != 0;) {
						switch (fd->cpp_type()) {

						case FieldDescriptor::CPPTYPE_DOUBLE:
						{
							double value = luaL_checknumber(L, -1);
							reflection->AddDouble(message, fd, value);
						}
						break;
						case FieldDescriptor::CPPTYPE_FLOAT:
						{
							float value = luaL_checknumber(L, -1);
							reflection->AddFloat(message, fd, value);
						}
						break;
						case FieldDescriptor::CPPTYPE_INT64:
						{
							int64_t value = luaL_checknumber(L, -1);
							reflection->AddInt64(message, fd, value);
						}
						break;
						case FieldDescriptor::CPPTYPE_UINT64:
						{
							uint64_t value = luaL_checknumber(L, -1);
							reflection->AddUInt64(message, fd, value);
						}
						break;
						case FieldDescriptor::CPPTYPE_ENUM: // 与int32一样处理
						{
							int32_t value = luaL_checknumber(L, -1);
							const EnumDescriptor* enumDescriptor = fd->enum_type();
							const EnumValueDescriptor* valueDescriptor = enumDescriptor->FindValueByNumber(value);
							reflection->AddEnum(message, fd, valueDescriptor);
						}
						break;
						case FieldDescriptor::CPPTYPE_INT32:
						{
							int32_t value = luaL_checknumber(L, -1);
							reflection->AddInt32(message, fd, value);
						}
						break;
						case FieldDescriptor::CPPTYPE_UINT32:
						{
							uint32_t value = luaL_checknumber(L, -1);
							reflection->AddUInt32(message, fd, value);
						}
						break;
						case FieldDescriptor::CPPTYPE_STRING:
						{
							size_t size = 0;
							const char* value = luaL_checklstring(L, -1, &size);
							reflection->AddString(message, fd, std::string(value, size));
						}
						break;
						case FieldDescriptor::CPPTYPE_BOOL:
						{
							bool value = lua_toboolean(L, -1);
							reflection->AddBool(message, fd, value);
						}
						break;
						case FieldDescriptor::CPPTYPE_MESSAGE:
						{
							Message* value = luaTableToProtobuf(L, lua_gettop(L), fd->message_type()->name().c_str());
							if (!value) {
								logError("convert to message %s failed whith value %s\n", fd->message_type()->name().c_str(), name.c_str());
								protocolManager::releaseMessage(value);
								return NULL;
							}
							Message* msg = reflection->AddMessage(message, fd);
							msg->CopyFrom(*value);
							protocolManager::releaseMessage(value);
						}
						break;
						default:
							break;
						}

						// remove value， keep the key
						lua_pop(L, 1);
					}
				}
				else {

					if (isRequired) {
						if (isNil) {
							logError("cant find required field %s\n", name.c_str());
							protocolManager::releaseMessage(message);
							return NULL;
						}
					}
					else {
						if (isNil) {
							lua_pop(L, 1);
							continue;
						}
					}

					switch (fd->cpp_type()) {
					case FieldDescriptor::CPPTYPE_DOUBLE:
					{
						double value = luaL_checknumber(L, -1);
						reflection->SetDouble(message, fd, value);
					}
					break;
					case FieldDescriptor::CPPTYPE_FLOAT:
					{
						float value = luaL_checknumber(L, -1);
						reflection->SetFloat(message, fd, value);
					}
					break;
					case FieldDescriptor::CPPTYPE_INT64:
					{
						int64_t value = luaL_checknumber(L, -1);
						reflection->SetInt64(message, fd, value);
					}
					break;
					case FieldDescriptor::CPPTYPE_UINT64:
					{
						uint64_t value = luaL_checknumber(L, -1);
						reflection->SetUInt64(message, fd, value);
					}
					break;
					case FieldDescriptor::CPPTYPE_ENUM: // 与int32一样处理
					{
						int32_t value = luaL_checknumber(L, -1);
						const EnumDescriptor* enumDescriptor = fd->enum_type();
						const EnumValueDescriptor* valueDescriptor = enumDescriptor->FindValueByNumber(value);
						reflection->SetEnum(message, fd, valueDescriptor);
					}
					break;
					case FieldDescriptor::CPPTYPE_INT32:
					{
						int32_t value = luaL_checknumber(L, -1);
						reflection->SetInt32(message, fd, value);
					}
					break;
					case FieldDescriptor::CPPTYPE_UINT32:
					{
						uint32_t value = luaL_checknumber(L, -1);
						reflection->SetUInt32(message, fd, value);
					}
					break;
					case FieldDescriptor::CPPTYPE_STRING:
					{
						size_t size = 0;
						const char* value = luaL_checklstring(L, -1, &size);
						reflection->SetString(message, fd, std::string(value, size));
					}
					break;
					case FieldDescriptor::CPPTYPE_BOOL:
					{
						bool value = lua_toboolean(L, -1);
						reflection->SetBool(message, fd, value);
					}
					break;
					case FieldDescriptor::CPPTYPE_MESSAGE:
					{
						Message* value = luaTableToProtobuf(L, lua_gettop(L), fd->message_type()->name().c_str());
						if (!value) {
							logError("convert to message %s failed whith value %s \n", fd->message_type()->name().c_str(), name.c_str());
							protocolManager::releaseMessage(message);
							return NULL;
						}
						Message* msg = reflection->MutableMessage(message, fd);
						msg->CopyFrom(*value);
						protocolManager::releaseMessage(value);
					}
					break;
					default:
						break;
					}
				}

				// pop value
				lua_pop(L, 1);
			}

			return message;
		}
		logError("cant find message  %s from compiled poll \n", name);
	}
	return NULL;
}

static int close(lua_State* L)
{
	Session* s = (Session*)tolua_touserdata(L, 1, 0);
	if (s)
		s->close();
	return 0;
}

static int getTag(lua_State* L)
{
	Session* s = (Session*)tolua_touserdata(L, 1, 0);
	if (s)
	{
		lua_pushinteger(L, s->utag);
		return 1;
	}
	return 0;
}

static int setTag(lua_State* L)
{
	Session* s = (Session*)tolua_touserdata(L, 1, 0);
	if (s)
	{
		int tag = lua_tointeger(L, 2);
		s->utag = tag;
	}
	return 0;
}


static int getUid(lua_State* L)
{
	Session* s = (Session*)tolua_touserdata(L, 1, 0);
	if (s)
	{
		lua_pushinteger(L, s->uid);
		return 1;
	}
	return 0;
}

static int setUid(lua_State* L)
{
	Session* s = (Session*)tolua_touserdata(L, 1, 0);
	if (s)
	{
		int uid = lua_tointeger(L, 2);
		s->uid = uid;
	}
	return 0;
}

static int getGid(lua_State* L)
{
	Session* s = (Session*)tolua_touserdata(L, 1, 0);
	if (s)
	{
		lua_pushinteger(L, s->gid);
		return 1;
	}
	return 0;
}

static int setGid(lua_State* L)
{
	Session* s = (Session*)tolua_touserdata(L, 1, 0);
	if (s)
	{
		int gid = lua_tointeger(L, 2);
		s->gid = gid;
	}
	return 0;
}

static int getAsClient(lua_State* L)
{
	Session* s = (Session*)tolua_touserdata(L, 1, 0);
	if (s)
	{
		lua_pushinteger(L, s->as_client);
		return 1;
	}
	return 0;
}

static int getAddress(lua_State* L)
{
	Session* s = (Session*)tolua_touserdata(L, 1, 0);
	if (s)
	{
		int port;
		const char* ip = s->getAddress(&port);
		lua_pushstring(L, ip);
		lua_pushinteger(L, port);
		return 2;
	}
	return 0;
}

static int sendMessage(lua_State* L)
{
	Session* s = (Session*)tolua_touserdata(L, 1, 0);
	if (s)
	{
		if (lua_istable(L, 2))
		{
			commandMessage msg;

			int n = luaL_len(L, 2);
			if (n == 4)
			{
				lua_pushnumber(L, 1);
				lua_gettable(L, 2);
				msg.serverType = luaL_checkinteger(L, -1);

				lua_pushnumber(L, 2);
				lua_gettable(L, 2);
				msg.commandType = luaL_checkinteger(L, -1);

				lua_pushnumber(L, 3);
				lua_gettable(L, 2);
				msg.tags = luaL_checkinteger(L, -1);

				lua_pushnumber(L, 4);
				lua_gettable(L, 2);


				if (protocolManager::getProtocolType() == PROTOCOL_JSON)
				{
					msg.body = (char*)lua_tostring(L, -1);
					s->sendMessage(&msg);
				}
				else	//protobuf
				{
					if (lua_istable(L, 6))
					{
						const char* msgName = protocolManager::getProtoCurrentMapName(msg.commandType);
						msg.body = luaTableToProtobuf(L, lua_gettop(L), msgName);
						s->sendMessage(&msg);
						protocolManager::releaseMessage((Message*)(msg.body));
					}
					else		//空值
					{
						msg.body = NULL;
						s->sendMessage(&msg);
					}

				}

			}
		}
		
	}
	return 0;
}

static int sendRawData(lua_State* L)
{
	Session* s = (Session*)tolua_touserdata(L, 1, 0);

	Gateway2Server* raw = (Gateway2Server*)tolua_touserdata(L, 2, 0);

	if (s)
		s->sendMessage(raw);

	return 0;
}

int regist_session_export(lua_State* L)
{
	//_G是lua中运行的全局表
	lua_getglobal(L, "_G");
	if (lua_istable(L, -1))
	{
		tolua_open(L);
		//表示类
		tolua_module(L, "Session", 0);
		tolua_beginmodule(L, "Session");
		//表示方法
		tolua_function(L, "close", close);
		tolua_function(L, "getTag", getTag);
		tolua_function(L, "setTag", setTag);
		tolua_function(L, "getUid", getUid);
		tolua_function(L, "setUid", setUid);
		tolua_function(L, "getGid", getGid);
		tolua_function(L, "setGid", setGid);
		tolua_function(L, "getAsclient", getAsClient);
		tolua_function(L, "close", close);
		tolua_function(L, "getAddress", getAddress);
		tolua_function(L, "sendMessage", sendMessage);
		tolua_function(L, "sendRawData", sendRawData);
		tolua_endmodule(L);
	}

	lua_pop(L, 1);
	return 0;
}
