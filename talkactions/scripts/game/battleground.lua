function onSay(cid, words, param)

	local _access = getPlayerAccess(cid)

	if(param == "") then
	
		local msg = "O uso deste comando requer um ou mais parametros. Exemplos:\n"
		msg = msg .. "!bg sobre, !bg info, !bg help -> Exibe informa��es sobre o sistema de Battlegrounds.\n"
	
		if(_access >= access.COMMUNITY_MANAGER) then
			msg = msg .. "!bg team team_id -> Exibe as informa��es de um time (1 ou 2).\n"
			msg = msg .. "!bg close -> Expulsa todos jogadores na battleground e a fecha.\n"
			msg = msg .. "!bg open -> Permite que jogadores entrem na battleground.\n"
			msg = msg .. "!bg stats -> Exibe as estatisticas (para voc�).\n"
			msg = msg .. "!bg statsall -> Exibe as estatisticas (para todos no canal).\n"
		else
			msg = msg .. "!bg team -> Exibe os membros de seu time (requer estar dentro da Battleground).\n"
		end
	
		pvpBattleground.sendPlayerChannelMessage(cid, msg)
		return TRUE
	end
	
	local explode = string.explode(param, " ", 1)
	
	option = explode[1]
	param = explode[2] or nil
	
	msg = ""
	
	if(isInArray({"info", "sobre", "help"}, option)) then	
		msg = msg .. "Informa��es sobre o sistema de Battlegrounds:\n"
		msg = msg .. pvpBattleground.getInformations()
	elseif(option == "team") then
	
		local team = nil 
		local error = false
		
		if(not doPlayerIsInBattleground(cid) and _access < access.COMMUNITY_MANAGER) then
			msg = msg .. "Para usar o comando \"!bg team\" � preciso estar dentro de uma Battleground."
			error = true
		end
		
		if not error then
			team = doPlayerGetBattlegroundTeam(cid)
		end
	
		if(_access >= access.COMMUNITY_MANAGER and isInArray({1, 2}, param)) then
			error = false
			team = param
		end
		
		if(not error) then
			msg = msg .. pvpBattleground.getPlayersTeamString(team)
		end
	elseif(option == "close" and _access >= access.COMMUNITY_MANAGER) then
		msg = msg .. "Battleground fechada."
		battlegroundClose()
	elseif(option == "open" and _access >= access.COMMUNITY_MANAGER) then
		msg = msg .. "Battleground aberta."
		battlegroundOpen()	
	elseif(option == "stats" and _access >= access.COMMUNITY_MANAGER) then
		pvpBattleground.broadcastStatistics(false, cid)
		return true
	elseif(option == "statsall" and _access >= access.COMMUNITY_MANAGER) then	
		pvpBattleground.broadcastStatistics(false)
		return true		
	end
	
	pvpBattleground.sendPlayerChannelMessage(cid, msg)
	return true
end
