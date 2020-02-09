local WSLuaCpp = require "WSLuaCpp"

GsUser = {}

GsUser.__index = GsUser

function GsUser:new(linkerId)
    local self = {}
    setmetatable(self,GsUser)
	
	self.linkerId = linkerId
	self.actorName = "user"..math.random(100000, 999999)
	self.sex = math.random(0, 1)
    
    return self
end

function GsUser:printInfo()
    local s = 'GsUser:printInfo linkerId:'..self.linkerId..' 角色名:'..self.actorName..'  性别:'..self.sex
    WSLuaCpp.showMsg(s)
end