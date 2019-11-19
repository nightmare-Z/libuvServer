Logger.init("log/gameServer", "gameServer", true)

local proto_type={
	json=0,
	protobuf=1,
	
}

protocolManager.init(proto_type.protobuf)
local Servers = require("ServiceServer/ServerConfig")
local Stype = require("serviceServer/ServerType")
local atService = require("serviceServer/gameServer/gameServer")
local commandMap = require("serviceServer/CommandName")

protocolManager.registerCmdMap(commandMap)

NetBus.startTcpServer(Servers.servers[Stype.System].ip, Servers.servers[Stype.System].port)

local ret = ServiceManager.registService(Stype.System,atService)

if ret == true then
	Logger.d("regist gameServer service success")
else
	Logger.d("regist gameServer service failed")
end