local mysql = require("../database/auth_mysql")
local redis = require("../database/auth_redis")
local Stype = require("serviceServer/ServerType")

function guestLogin(s,msg) 
	local guestKey = msg[4].randkey
	mysql.get_guestInfo(guestKey,function(err , info)  
		--出错饭反馈给客户端
		if err then	
			Logger.d("mysql select func error")
			Logger.d(err)
			return
		end
		--key对应的用户不在当中

		if info == nil then
			mysql.regist_Guest(guestKey,function(err,ret) 
				--注册出错
				if err then	
					Logger.d(err)
					Logger.d("mysql insert func err")
					Session.sendMessage(s,{Stype.Auth,msg[2]+1,msg[3],{guestError = -1}})
					return
				end
				login(s,msg)
				
			end)
			return
		end
		
		local res = {}

		----找到了游客的数据
		if tonumber(info.status)  ~= 0 then
			Logger.d("account is illegelity")
			Session.sendMessage(s,{Stype.Auth,msg[2]+1,msg[3],{guestError = -3}})			
		else		
			Logger.d("guest info get")
			
			--将数据丢给redis
			redis.set_guestInfo(info.guestId ,info)
			
			Session.sendMessage(s,{Stype.Auth , msg[2]+1 , msg[3] , 
			{guestError = 1,
			info = {nick = info.nick,
					faceImage = info.faceImage, 
					vip = info.vip,
					status =info.status,
					guestId = info.guestId,}
			}})
		end
		
		
	end) 
	
end

local guest = {
	login = guestLogin,
}

return guest