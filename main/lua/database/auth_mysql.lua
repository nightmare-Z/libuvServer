local gameConfig = require("../serviceServer/ServerConfig")
local Error = require("serviceServer/ErrorType")
local mysql_conn = nil

function mysqlToAuth()
	local authConfig = gameConfig.auth_mysql
	mysqlManager.connect(authConfig.host,authConfig.port,authConfig.dbName,authConfig.user,authConfig.password,function(err,conn)
		if err then
			Logger.e(err)
			scheduler.scheduleOnce(mysqlToAuth,5000)
			return
		end
		Logger.d("connect to mysql database success !!!")
		mysql_conn = conn
	end)
end

mysqlToAuth()

function Logger.dTable( tbl , level, filteDefault)
  local msg = ""
  filteDefault = filteDefault or true --默认过滤关键字（DeleteMe, _class_type）
  level = level or 1
  local indent_str = ""
  for i = 1, level do
    indent_str = indent_str.."  "
  end

  Logger.d(indent_str .. "{")
  for k,v in pairs(tbl) do
    if filteDefault then
      if k ~= "_class_type" and k ~= "DeleteMe" then
        local item_str = string.format("%s%s = %s", indent_str .. " ",tostring(k), tostring(v))
        Logger.d(item_str)
        if type(v) == "table" then
          Logger.dTable(v, level + 1)
        end
      end
    else
      local item_str = string.format("%s%s = %s", indent_str .. " ",tostring(k), tostring(v))
      Logger.d(item_str)
      if type(v) == "table" then
        Logger.dTable(v, level + 1)
      end
    end
  end
  Logger.d(indent_str .. "}")
end

function getGuestInfo(gKey,retFunc)
	
	if mysql_conn == nil then
		if retFunc then
			retFunc("mysql is not connect",nil)	
		end
		return
	end
	
	if retFunc == nil then
		return
	end
	
	local sqlsm = "select nick , faceImage , status ,vip ,guestId from guest where guest_key = '%s';"
	
	local finalsm = string.format(sqlsm,gKey)
	
	mysqlManager.query(mysql_conn,finalsm,function(err,ret) 
		--语句错误
		if err then
			retFunc(err,nil)
			return
		end
		
		--数据库没有 ret的第一个表的我用来提示表的列名 数据在之后
		if ret == nil or #ret <= 1 then
			retFunc(nil,nil)
			return
		end
		--查询成功 0]拿到数据0h
		--Logger.dTable(ret)
		local info = {}
		
		for k , v in pairs(ret[2]) do
			info[ret[1][k]] = v
		end
		
		retFunc(nil,info)
	end)
end

function registGuest(gKey,retFunc)
	if mysql_conn == nil then
		if retFunc then
			retFunc("mysql is not connect",nil)	
		end
		return
	end
	
	math.randomseed(os.time())
	local unick = "guest"..math.random(100000,999999)
	local face = math.random(1,200)
	local sqlsm = "insert into guest (nick, faceImage , status, guest_key)values('%s',%d,%d,%d,'%s');"
	local finalsm = string.format(sqlsm , unick , face , 0 , gKey)
	
	mysqlManager.query(mysql_conn,finalsm,function(err,ret) 
		--语句错误
		if err then
			retFunc(err,nil)
			return
		else
			retFunc(nil,nil)
		end
	end)
end

function upgrade_guestAccount(username , password ,retFunc)
	local exist = false
	if mysql_conn == nil then
		if retFunc then
			retFunc("mysql is not connect",nil)	
		end
		return
	end
	
	local sqlsm = "select account from primaryInfo where account = '%s';"
	local finalsm = string.format(sqlsm , username)
	
	mysqlManager.query(mysql_conn,finalsm,function(err,ret) 
		--语句错误
		if err then
			retFunc(err,nil)
			return
		end
		
		if ret[2] ~= nil and #ret[2] > 0 then
			retFunc(nil,Error.eGuestUserNameExist)
			exist = true
			return
		end
	
	end)
	
	if exist then
		return
	end
	
	local sqlsm = "insert into primaryInfo (account, password)values('%s','%s');"
	local finalsm = string.format(sqlsm , username , password , 0 )
	mysqlManager.query(mysql_conn,finalsm,function(err,ret) 
		--语句错误
		if err then
			retFunc(err,nil)
			return
		else
			retFunc(nil,"upgradeGuestAccount success")
			return
		end
	end)
	
end

local mysql_Center = {
	get_guestInfo = getGuestInfo,
	regist_Guest = registGuest,
	upgrade_guestAccount = upgradeGuestAccount
}

return mysql_Center