
cclog = function(...)   -- geneserver.exe使用
    WSLuaCpp.showMsg(string.format(...))
end

-- 获取表的最大与最小索引
function getTableIndex(tb)       
    local temptb = {}
    local Min,Max = 0,0
    for k,v in pairs(tb) do 
        table.insert(temptb,k)             
    end
    table.sort(temptb)
    if #temptb ~= 0 then
        Min,Max = temptb[1],temptb[#temptb]
    end    
    return Min,Max
end

--获取当前系统时间
function getNowTime()   
    local strTime = os.date("%Y-%m-%d %X")
    return strTime
end

-- 获取时间间隔秒数(若获取距离当前时间的间隔strTime2='')
function getSecondsInter(strTime1,strTime2)
    local temp = {}
    local temp2 = {}
    temp.year   = string.sub(strTime1,1,4)
    temp.month  = string.sub(strTime1,6,7)
    temp.day    = string.sub(strTime1,9,10)
    temp.hour   = string.sub(strTime1,12,13)
    temp.min    = string.sub(strTime1,15,16)
    temp.sec    = string.sub(strTime1,18,19)

    if temp.year == '0000' then
        return 0
    end  
    if strTime2 ~= '' then
        temp2.year   = string.sub(strTime2,1,4)
        temp2.month  = string.sub(strTime2,6,7)
        temp2.day    = string.sub(strTime2,9,10)
        temp2.hour   = string.sub(strTime2,12,13)
        temp2.min    = string.sub(strTime2,15,16)
        temp2.sec    = string.sub(strTime2,18,19)
        
        local ti = os.time(temp)
        local nowSec = os.time(temp2)

        return (nowSec - ti)
    else
        local ti = os.time(temp)
        local nowSec = os.time()

        return (nowSec - ti)
    end
    
end

-- 改变时间(分,秒)
function changeTime(strTime,nMin,nSec)
    local temp = {}
    temp.year   = string.sub(strTime,1,4)
    temp.month  = string.sub(strTime,6,7)
    temp.day    = string.sub(strTime,9,10)
    temp.hour   = string.sub(strTime,12,13)
    temp.min    = string.sub(strTime,15,16) + nMin
    temp.sec    = string.sub(strTime,18,19) + nSec

    local ti = os.time(temp)
    return os.date("%Y-%m-%d %X",ti)
end

function GetTableValidItemNum(tb)
    local nNum = 0
    for k,v in pairs(tb) do
        if v ~= nil then
            nNum = nNum + 1
        end
    end
    return nNum
end

function StringSplit(allStr,splitStr)
    local subStrTbl = {}
	if string.len(allStr) == 0 then
		return subStrTbl
	end
	
    while(true) do    
        local pos = string.find(allStr,splitStr)        
        if (pos == nil) then
            local size_t = table.getn(subStrTbl)
            table.insert(subStrTbl,size_t + 1,allStr)
            break
        end   
        local subStr = string.sub(allStr,1,pos -1)
        local size_t = table.getn(subStrTbl)
        table.insert(subStrTbl,size_t + 1,subStr)
        local t = string.len(allStr)
        allStr = string.sub(allStr,pos + 1,t)
        t = string.len(allStr)
        if t == 0 then break end
    end
    
    return subStrTbl
end

function ReadOnly(t)
    local proxy = {}
    local mt = {        -- 创建元表
        __index = t,
        __newindex = function (t,k,v)
            error("error,attemp to update a read-only table",2)
        end
    }
    setmetatable(proxy,mt)
    return proxy
end

--[[
client向gamesvr发送消息
]]--
function SendMsgToGameSvr(netId,msgId,strCmd,...)    
    local arg = {...}
    arg.n = select ("#",...)

    local nStrLen = string.len(strCmd)
    local nArgLen = arg.n
    if nStrLen ~= nArgLen then
        error("SendMsgToGameSvr.args error",2)
        return   
    end

    WSLuaCpp.beginSendBuf(netId,msgId)
    for i = 1,nStrLen,1 do
        local sTemp = string.sub(strCmd,i,i)
        if sTemp == "b" then
            WSLuaCpp.writeSendByte(arg[i])
        elseif sTemp == "i" then
            WSLuaCpp.writeSendInt(arg[i])
        elseif sTemp == "h" then
            WSLuaCpp.writeSendShort(arg[i])
        elseif sTemp == "u" then
            WSLuaCpp.writeSendUint32(arg[i])
        elseif sTemp == "g" then
            WSLuaCpp.writeSendDouble(arg[i])
        elseif sTemp == "s" then
            WSLuaCpp.writeSendString(arg[i])
        end
    end
    WSLuaCpp.endSendBuf()
    
	WSLuaCpp.sendDBufToGameSvr()
end

--[[
gamesvr向客户端发消息
]]--
function SendMsgToClient(netId,msgId,strCmd,...)    
    local arg = {...}
    arg.n = select ("#",...)

    local nStrLen = string.len(strCmd)
    local nArgLen = arg.n
    if nStrLen ~= nArgLen then
        error("SendMsgToClient.args error",2)
        return   
    end

    WSLuaCpp.beginSendBuf(netId,msgId)
    for i = 1,nStrLen,1 do
        local sTemp = string.sub(strCmd,i,i)
        if sTemp == "b" then
            WSLuaCpp.writeSendByte(arg[i])
        elseif sTemp == "i" then
            WSLuaCpp.writeSendInt(arg[i])
        elseif sTemp == "h" then
            WSLuaCpp.writeSendShort(arg[i])
        elseif sTemp == "u" then
            WSLuaCpp.writeSendUint32(arg[i])
        elseif sTemp == "g" then
            WSLuaCpp.writeSendDouble(arg[i])
        elseif sTemp == "s" then
            WSLuaCpp.writeSendString(arg[i])
        end
    end
    WSLuaCpp.endSendBuf()
    
	WSLuaCpp.sendDBufToClient(netId)
end

-- for CCLuaEngine traceback
function Throw(msg)
    cclog("----------------------------------------")
    cclog("LUA ERROR: " .. tostring(msg) .. "\n")
    cclog(debug.traceback())
    cclog("----------------------------------------")
    return msg
end

--递归将一个table转换成string
function printTable(table)
  if type(table) == "table" then
    local string = ""
      for k,v in pairs(table) do
          string = string .. "{" .. k .. "=" ..  printTable(v) .."}, \n"
      end
      return string 
    else
      return string.gsub(table, "%%", " ")     
  end
end
