
GLOBAL_TABLE_TYPE_MISC = 0
GLOBAL_TABLE_TYPE_TABLES = 1

local GlobalTable_mt = {
	__index = function(o, key)
		return getGlobalListValue(o.__name, key)
	end,
	__newindex = function(o, key, val)
		setGlobalListValue(o.__name, key, val)
	end
}
 
function global_table(name)
	local t = {}
	local guid = useGlobalList(name)
	t.__name = guid
	setmetatable(t, GlobalTable_mt)
	return t
end

function useGlobalList(name)

	local result = db.getResult("SELECT `id` FROM `lua_global_table` WHERE `name` = '".. name .."';")
	
	local guid = nil
	
	if(result:getID() ~= -1) then
		guid = result:getDataInt("id")
		result:free()
		
		return guid
	end	

	db.executeQuery("INSERT INTO `lua_global_table` (`name`) VALUES ('" .. name .. "');")
	return db.lastInsertId()
end

function getGlobalListValue(name, key)
	local result = db.getResult("SELECT `v`.`value` FROM `lua_global_value` `v` LEFT JOIN `lua_global_table` `t` ON `t`.`id` = `v`.`table_id` WHERE `t`.`name` = '".. name .."' AND `v`.`key` = '" .. key .. "';")
	
	local value = nil
	
	if(result:getID() ~= -1) then
		value = result:getDataString("value")
		result:free()
	end	
	
	return value
end

function setGlobalListValue(name, key, value)

	if(getGlobalListValue(name, key) == nil) then
		db.executeQuery("INSERT INTO `lua_global_value` (`table_id`, `key`, `value`) VALUES ((SELECT `id` FROM `lua_global_table` WHERE `name` = '" .. name .. "'), '" .. key .. "', '".. value .. "');")
	else
		db.executeQuery("UPDATE `lua_global_value` `v` LEFT JOIN `lua_global_table` `t` ON `v`.`table_id` = `t`.`id` SET `value` = '".. value .. "' WHERE `t`.`name` = '" .. name .. "' AND `v`.`key` = '" .. key .. "';")
	end
end

luaGlobal = {}

function luaGlobal.getVar(name)
	local result = db.getResult("SELECT `value` FROM `lua_global` WHERE `var` = '" .. name .. "';")
	local value = nil
	
	if(result:getID() ~= -1) then
		value = result:getDataString("value")
		result:free()
	end	
	
	if(value ~= nil) then
		local json = require("json")
		value = json.decode(value)	
	end
	
	return value
end

function luaGlobal.setVar(var, value)
	
	local result = db.getResult("SELECT `value` FROM `lua_global` WHERE `var` = '" .. var .. "';")
	
	
	local json = require("json")	
	value = json.encode(value)		
	
	if(result:getID() ~= -1) then
		result:free()
		db.executeQuery("UPDATE `lua_global` SET `value` = '" .. value .. "' WHERE `var` = '" .. var .. "';")	
	else
		db.executeQuery("INSERT INTO `lua_global` VALUES ('" .. var .. "', '" .. value .. "');")
	end
	
end

function luaGlobal.unsetVar(var)
	db.executeQuery("DELETE FROM `lua_global` WHERE `var` = '" .. var .. "';")	
end

function luaGlobal.truncate()
	db.executeQuery("TRUNCATE `lua_global`;")
end