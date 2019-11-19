local mysql = require("../database/auth_mysql")
local redis = require("../database/auth_redis")
local Stype = require("serviceServer/ServerType")

function guestUpgrade(s,msg) 
	local body = msg[4]
	local username = body.username
	local passWord = body.passWord
	
	if userName and passWord then
		mysql.upgrade_guestAccount(userName , passWord , function(err,ret) 
			if err then 
				Logger.e(err)
				return
			end
			
			if type(ret) == "number" then
				Session.sendMessage(s,{msg[1],msg[2]+1,msg[3],{status = ret}})
			elseif type(ret) == "string" then
				Session.sendMessage(s,{msg[1],msg[2]+1,msg[3],{status = 1}})
			end
		
		
		end)
	
	end
	
end

local guest_upgrade = {
	upgrade = guestUpgrade,
}

return guest_upgrade