require "common.Functions"

Singleton = {}
Singleton.__index = Singleton

function Singleton:Instance()
    if self.instance == nil then
        local inst = {}
        setmetatable(inst,Singleton)
        
        inst.para1 = 1
        inst.para2 = 2
        self.instance = inst
    end
    return self.instance
end

function Singleton:printInfo()
    cclog(self.para1..' '..self.para2)
end