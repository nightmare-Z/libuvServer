Logger.init("log/auth", "auth", true)

local proto_type={
	json=0,
	protobuf=1,
	
}

protocolManager.init(proto_type.protobuf)
local Servers = require("ServiceServer/ServerConfig")
local Stype = require("serviceServer/ServerType")
local atService = require("serviceServer/auth/auth")
local commandMap = require("serviceServer/CommandName")

protocolManager.registerCmdMap(commandMap)

NetBus.startTcpServer(Servers.servers[Stype.Auth].ip, Servers.servers[Stype.Auth].port)

local ret = ServiceManager.registService(Stype.Auth,atService)

if ret == true then
	Logger.d("regist auth service success")
else
	Logger.d("regist auth service failed")
end