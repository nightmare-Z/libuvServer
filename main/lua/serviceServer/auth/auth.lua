local stype  = require("serviceServer/ServerType")
local ctype  = require("serviceServer/CommandType")
local guestLogin = require("serviceServer/auth/guestAuth/guestLogin")
local guestUpgrade = require("serviceServer/auth/guestAuth/guestUpgrade")

local auth = {}

auth[ctype.eGuestLoginReq] = guestLogin.login
auth[ctype.eGuestUpgradeReq] = guestUpgrade.upgrade

function at_recv_cmd(s,msg)
	if auth[msg[2]] ~= nil then
		auth[msg[2]](s,msg)
	end
end

function at_disc_session(s , stype)

end

local at_server = {
	on_recv = at_recv_cmd,
	on_discon = at_disc_session,
}

return at_server