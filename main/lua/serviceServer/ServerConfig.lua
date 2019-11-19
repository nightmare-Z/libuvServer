local Stype = require("serviceServer/ServerType")
local remoteServers = {}

remoteServers[Stype.Auth] = {
	stype = Stype.Auth,
	ip = "127.0.0.1",
	port = 975,
	desic = "Auth server",
}

remoteServers[Stype.System] ={
	stype = Stype.System,
	ip = "127.0.0.1",
	port = 796,
	desic = "System server",
}


local ServerConfig = {
	getway_tcp_ip = "127.0.0.1",
	getway_tcp_port = 1270,
	
	getway_web_ip = "127.0.0.1",
	getway_web_port = 1271,
	
	servers = remoteServers,
	
	auth_mysql = {
		host = "127.0.0.1",
		port = 3306,
		dbName = "nightmare",
		user = "root",
		password = "s1270975796",
	},
	
	auth_redis = {
		host = "127.0.0.1",
		port = 6379,
		dbIndex = 1,
	},
}

return ServerConfig