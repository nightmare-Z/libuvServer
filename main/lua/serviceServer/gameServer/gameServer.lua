local stype  = require("serviceServer/ServerType")
local ctype  = require("serviceServer/CommandType")


local system = {}

--system[ctype.eLoginReq] = guestLogin.login

function at_recv_cmd(s,msg)
	if system[msg[2]] ~= nil then
		system[msg[2]](s,msg)
	end
end

function at_disc_session(s , stype)

end

local at_server = {
	on_recv = at_recv_cmd,
	on_discon = at_disc_session,
}

return at_server