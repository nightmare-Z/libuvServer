local otherServer = require("serviceServer/ServerConfig")
local Ctype = require("serviceServer/CommandType")
local Stype = require("serviceServer/ServerType")
local GameError = require("serviceServer/ErrorType")
--映射  根据stype的值 找到要转发的session
local sType2Session = {}
--映射  根据导入的服务器配置表 保存其他服务器的连接状态
local do_connecting = {}


--临时的全局tag 
local gKey = 1
--保存全局的session映射   注 ：未登录的会话也算在内 否则则丢失掉
local client_temp = {}
--游客会话表
local client_gSession = {}
--保存用户全局的注册session映射
local client_uid = {}
--保存用户全局的游客session映射
local client_gid = {}



--网关服务连接其他服务
function connect(stype , desic , ip , port)
	NetBus.getawayConnect(ip , port , function(err,session)
		--连接状态的回调
		do_connecting[stype] = true
		--连接失败
		if err ~= 0 then
			do_connecting[stype] = false
			return
		end
		--连接成功
		sType2Session[stype] = session
		Logger.d("gateway connect.."..desic..".."..ip..":"..port.." Success")
	end)

end

--循环检查在服务器配置表中 已有的服务器但是没有连接上的
function checkConnected() 
	for k , v in pairs(otherServer.servers) do 
		if sType2Session[v.stype] == nil and do_connecting[v.stype] == false then
			do_connecting[v.stype] = true 
			connect(v.stype,v.desic,v.ip,v.port)
			Logger.d("gateway connect.."..v.desic..".."..v.ip..":"..v.port)
		end
	end
end 
--网关服务器的装载函数
function gw_init()
	for k , v in pairs(otherServer.servers) do
		sType2Session[v.stype] = nil
		do_connecting[v.stype] = false
	end
	--无限循环去检查服务器状态
	scheduler.scheduleRepeat(checkConnected, 1000 , -1 , 5000)
	
end

--转发数据到其他服务器
function sendToServer(client_s , raw)
	local stype , ctype , utag = Raw.readHead(raw)
	--其他服务器注册表中不存在  则没有这个stype的服务器在使用
	if sType2Session[stype] == nil then
		Logger.e("Server "..stype.." Not Run!!!")
		return
	end
	
	--如果是登录请求 那么要先给会话一个标识符
	if ctype == Ctype.eGuestLoginReq then 
		utag = Session.getTag(client_s)
		if utag == 0 then
			utag =gKey
			gKey = gKey + 1
			Logger.d("alloction gkey")
			Session.setTag(client_s , utag)
			Session.setGid(client_s , utag)
		end
		--在临时映射表添加这个啥标记都没有的新会话
		client_temp[utag] = client_s
	elseif ctype == Ctype.eLoginReq then
		utag = Session.getTag(client_s)
		if utag == 0 then
			utag =gKey
			gKey = gKey + 1
			Logger.d("alloction gkey")
			Session.setTag(client_s , utag)
			Session.setUid(client_s , utag)
		end
		--在临时映射表添加这个啥标记都没有的新会话
		client_temp[utag] = client_s
	else
		local uid = Session.getUid(client_s)
		local gid = Session.getGid(client_s)
		if uid ~= 0 then
			utag = uid
			client_uid[uid] = client_s
		end
		
		if gid ~= 0 then
			utag = gid
			client_gid[gid] = client_s
		end
		--不是登录的请求，然后又没有id 直接无视
		if uid == 0 and gid == 0 then
			Logger.d("uesr reqestion is illegality")
			return
		end
		-- 
	end
	
	--把用户的uid 打到命令包里面 在转发到命令包解出来的对应的服务器session
	Raw.setTag(raw,utag)
	Session.sendRawData(sType2Session[stype] , raw)
	
end

--转发数据到客户端
function sendToClient(server_s , raw)
	local stype , ctype , utag = Raw.readHead(raw)
	local client_s_user = nil
	local client_s_guest = nil
	--如果是服务器回给客户端的登录命令 那么就获取这个客户端的唯一userid给这个session一个验证过的身份
	if ctype == Ctype.eGuestLoginRes then
		local rawBody = Raw.readBody(raw)

		for k , v in pairs(rawBody.info) do
			print(k)
		end
		client_s_guest = client_temp[utag]
		--登录返回之后  说明它是一个有身份的会话了  就把它从临时会话表中移除掉
		client_temp[utag] = nil
		
		if client_s_guest == nil then
			Logger.e("client_s_guest  session is null")
			return
		end
		
		if tonumber(rawBody.guestError) ~= 1 then			
			Raw.setTag(raw,0)
			if client_s_guest then
				Session.sendRawData(client_s_guest,raw)
				return
			end
		end
		
		local guestId = rawBody.info.guestId
		--表示已经登录
		if client_gid[guestId] and client_gid[guestId] ~= client_s_guest then
			Session.sendMessage(client_gid[guestId],{Stype.Auth,ctype,0,{guestError = -5}})
			Session.close(client_gid[guestId])
			client_gid[guestId] = nil
		end
		
		client_gid[guestId] = client_s_guest
		Session.setGid(client_s_guest,guestId)
		rawBody.info.guestId = 0
		
		Session.sendMessage(client_s_guest,{stype,ctype,0,{guestError = rawBody.guestError, info = rawBody.info}})
		return		
	elseif ctype == Ctype.eLoginRes then
		local rawBody = Raw.readBody(raw)
		
		client_s_user = client_temp[utag]
		--登录返回之后  说明它是一个有身份的会话了  就把它从临时会话表中移除掉
		client_temp[utag] = nil
		
		if client_s_user == nil then
			Logger.e("client_s_user  session is null")
			return
		end
		
		if tonumber(rawBody.guestError) ~= 1 then			
			Raw.setTag(raw,0)
			if client_s_user then
				Session.sendRawData(client_s_user,raw)
				return
			end
		end
		
		local userid = rawBody.info.userid
		--表示已经登录
		if client_uid[userid] and client_uid[userid] ~= client_s_user then
			Session.sendMessage(client_uid[userid],{Stype.Auth,ctype,0,{guestError = -5}})
			Session.close(client_uid[userid])
			client_uid[userid] = nil
		end
		client_uid[userid] = client_s_user
		Session.setUid(client_s_user,userid)
		rawBody.info.userid = 0
		
		Session.sendMessage(client_s_user,{stype,ctype,0,{guestError = rawBody.guestError, info = rawBody.info}})
		return
	end
	
	client_s_user = client_uid[tag]
	client_s_guest = client_gid[tag]
	
	if client_s_user then
		local uid = Session.getUid(client_s)
		if uid ~= 0 then
			Raw.setTag(raw,0)
			Session.sendRawData(client_s_user,raw)
		end
	end
	
	client_s_guest = client_gid[tag]
	if client_s_guest then
		local gid = Session.getGid(client_s)
		if gid ~= 0 then
			Raw.setTag(raw,0)
			Session.sendRawData(client_s_guest,raw)
		end
	end
	
	
	
	
	
	
end

--网关注册的转发服务
function gw_repeaterSession(s,raw)
	if Session.getAsclient(s) == 0 then
		Logger.d("gateway into server")
		sendToServer(s , raw)
	else
		Logger.d("gateway into client")
		sendToClient(s , raw)
	end
end

function gw_disc_session(s , stype)
	--getAsclient  1 则标识为服务器  0  则是客户端
	--如果有服务端session断线  那么就重新改变下连接状态
	if Session.getAsclient(s) == 1 then
		for k , v in pairs(sType2Session) do
			if v == s then
				Logger.d("gateway disconnect-->"..otherServer.servers[k].desic)
				sType2Session[k] = nil
				do_connecting[k] = false
				return
			end
		end
		return
	end
	--如果有客户端session断线  那么就在我的临时表中移除这个session

	local utag = Session.getTag(s)
	if client_temp[utag] ~= nil and client_temp[utag] == s then
		client_temp[utag] = nil
		Session.setTag(s,0)
		Logger.d("temporary session by client_temp")
	end
	
	local uid = Session.getUid(s)
	if client_uid[uid] ~= nil and client_temp[uid] == s then
		Logger.d("official session by client_temp")
		client_uid[uid] = nil
	end
	
	--玩家离线 广播给其他服务器
	if uid ~= 0 then
		Logger.d("official session is disconnected")
		local userLost = {stype , GameError.eUserLoseConn ,  uid , nil}
		Session.sendMessage(s , userLost)
	end
	
end

gw_init()

local gw_server = {
	repeatCmd = gw_repeaterSession,
	gw_disc = gw_disc_session,
}

return gw_server