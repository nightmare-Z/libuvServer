local gameConfig = require("../serviceServer/ServerConfig")

local redis_conn = nil

function redisToAuth()
	local authConfig = gameConfig.auth_redis
	redisManager.connect(authConfig.host,authConfig.port,function(err,conn)
		if err then
			Logger.e(err)
			scheduler.scheduleOnce(redisToAuth,5000)
			return
		end
		
		Logger.d("connect to redis database success !!!")
		
		redis_conn = conn
		
		redisManager.query(redis_conn ,"select "..authConfig.dbIndex, function(err,ret) 
			if err then
				Logger.e(err)
				return
			end
			Logger.d("select to redis index "..authConfig.dbIndex.." success !!!")
		end)
	end)
end

redisToAuth()

function getGuestInfo(gid ,retFunc)
	local redisState = "hgetall guest_"..gid

	redisManager.query(redis_conn , redisState , function(err,ret) 

		if err then
			retFunc(err,nil)
			return
		end

		local i = 1
		local redisRet = {}
		while i < #ret do
			redisRet[ret[i]] = ret[i+1]
			i = i + 2
		end
		
		retFunc(nil,redisRet)

	end)

end

function setGuestInfo(gid , uinfo) 
	local	redisState = "hmset guest_"..gid.." nick "..uinfo.nick..
											" faceImage "..uinfo.faceImage..
											" vip "..uinfo.vip..
											" status "..uinfo.status
											
	
	redisManager.query(redis_conn , redisState , function(err,ret) 
		if err then
			Logger.e(err)
		end
		Logger.d("redis query set success !!")
		
	end)
	
end

--
function updateGuestInfo(gid, nick , faceImage , vip , status )
	getGuestInfo(gid , function(err,uinfo) 
		if err then
			Logger.e("update info befor get info error")
			return
		end

		uinfo.nick      = nick 
		uinfo.faceImage = faceImage
		uinfo.vip       = vip
		uinfo.status    = status 

		setGuestInfo(uinfo.gusetId,uinfo)
		return
	end)
end

local redis_Center = {
	get_guestInfo = getGuestInfo,
	set_guestInfo = setGuestInfo,
	update_guestInfo = updateGuestInfo,
}

return redis_Center