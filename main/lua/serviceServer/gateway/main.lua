Logger.init("log/gateway", "gateway", true)

local proto_type={
	json=0,
	protobuf=1,
	
}

protocolManager.init(proto_type.protobuf)

local Servers = require("serviceServer/ServerConfig")
local gwService = require("serviceServer/gateway/gateway")
local commandMap = require("serviceServer/CommandName")

protocolManager.registerCmdMap(commandMap)
NetBus.startTcpServer(Servers.getway_tcp_ip, Servers.getway_tcp_port)
NetBus.startWebServer(Servers.getway_web_ip, Servers.getway_web_port)

for k , v in pairs(Servers.servers) do
	local ret = ServiceManager.repeaterService(v.stype,gwService)
	if ret == true then
		Logger.d("regist getway "..v.stype.." service success")
	else
		Logger.d("regist getway "..v.stype.." service default")
	end
end
