////////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
////////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////
#include "otpch.h"
#ifdef __DEBUG_LUASCRIPTS__
#include <sstream>
#endif

#include "creatureevent.h"
#include "player.h"
#include "tools.h"

#ifdef __DARGHOS_PVP_SYSTEM__
extern Battleground g_battleground;
#endif

CreatureEvents::CreatureEvents():
m_interface("CreatureScript Interface")
{
	m_interface.initState();
}

CreatureEvents::~CreatureEvents()
{
	for(CreatureEventList::iterator it = m_creatureEvents.begin(); it != m_creatureEvents.end(); ++it)
		delete (*it);

	m_creatureEvents.clear();
}

void CreatureEvents::clear()
{
	//clear creature events
	for(CreatureEventList::iterator it = m_creatureEvents.begin(); it != m_creatureEvents.end(); ++it)
		(*it)->clearEvent();

	//clear lua state
	m_interface.reInitState();
}

Event* CreatureEvents::getEvent(const std::string& nodeName)
{
	std::string tmpNodeName = asLowerCaseString(nodeName);
	if(tmpNodeName == "event" || tmpNodeName == "creaturevent" || tmpNodeName == "creatureevent" || tmpNodeName == "creaturescript")
		return new CreatureEvent(&m_interface);

	return NULL;
}

bool CreatureEvents::registerEvent(Event* event, xmlNodePtr, bool override)
{
	CreatureEvent* creatureEvent = dynamic_cast<CreatureEvent*>(event);
	if(!creatureEvent)
		return false;

	if(creatureEvent->getEventType() == CREATURE_EVENT_NONE)
	{
		std::clog << "[Error - CreatureEvents::registerEvent] Trying to register event without type!" << std::endl;
		return false;
	}

	if(CreatureEvent* oldEvent = getEventByName(creatureEvent->getName()))
	{
		//if there was an event with the same type that is not loaded (happens when realoading), it is reused
		if(oldEvent->getEventType() == creatureEvent->getEventType())
		{
			if(!oldEvent->isLoaded() || override)
				oldEvent->copyEvent(creatureEvent);

			return override;
		}
	}

	//if not, register it normally
	m_creatureEvents.push_back(creatureEvent);
	return true;
}

CreatureEvent* CreatureEvents::getEventByName(const std::string& name)
{
	for(CreatureEventList::iterator it = m_creatureEvents.begin(); it != m_creatureEvents.end(); ++it)
	{
		if((*it)->getName() == name)
			return (*it);
	}

	return NULL;
}

bool CreatureEvents::playerLogin(Player* player)
{
	//fire global event if is registered
	bool result = true;
	for(CreatureEventList::iterator it = m_creatureEvents.begin(); it != m_creatureEvents.end(); ++it)
	{
		if((*it)->getEventType() == CREATURE_EVENT_LOGIN && (*it)->isLoaded()
			&& !(*it)->executeLogin(player) && result)
			result = false;
	}

	return result;
}

bool CreatureEvents::playerLogout(Player* player, bool forceLogout)
{
	#ifdef __DARGHOS_PVP__
	// em teoria um player nunca conseguiria deslogar dentro de uma battleground, mas.. vai saber se por acidente alguem d� um /closeserver nao � verdade?
	if(player->isInBattleground())
	{
		g_battleground.kickPlayer(player, true);
	}
	#endif

	//fire global event if is registered
	bool result = true;
	for(CreatureEventList::iterator it = m_creatureEvents.begin(); it != m_creatureEvents.end(); ++it)
	{
		if((*it)->getEventType() == CREATURE_EVENT_LOGOUT && (*it)->isLoaded()
			&& !(*it)->executeLogout(player, forceLogout) && result)
			result = false;
	}

	return result;
}

/////////////////////////////////////

CreatureEvent::CreatureEvent(LuaInterface* _interface):
Event(_interface)
{
	m_type = CREATURE_EVENT_NONE;
	m_isLoaded = false;
}

bool CreatureEvent::configureEvent(xmlNodePtr p)
{
	std::string str;
	if(!readXMLString(p, "name", str))
	{
		std::clog << "[Error - CreatureEvent::configureEvent] No name for creature event." << std::endl;
		return false;
	}

	m_eventName = str;
	if(!readXMLString(p, "type", str))
	{
		std::clog << "[Error - CreatureEvent::configureEvent] No type for creature event." << std::endl;
		return false;
	}

	std::string tmpStr = asLowerCaseString(str);
	if(tmpStr == "login")
		m_type = CREATURE_EVENT_LOGIN;
	else if(tmpStr == "logout")
		m_type = CREATURE_EVENT_LOGOUT;
	else if(tmpStr == "joinchannel")
		m_type = CREATURE_EVENT_CHANNEL_JOIN;
	else if(tmpStr == "leavechannel")
		m_type = CREATURE_EVENT_CHANNEL_LEAVE;
	else if(tmpStr == "advance")
		m_type = CREATURE_EVENT_ADVANCE;
	else if(tmpStr == "sendmail")
		m_type = CREATURE_EVENT_MAIL_SEND;
	else if(tmpStr == "receivemail")
		m_type = CREATURE_EVENT_MAIL_RECEIVE;
	else if(tmpStr == "traderequest")
		m_type = CREATURE_EVENT_TRADE_REQUEST;
	else if(tmpStr == "tradeaccept")
		m_type = CREATURE_EVENT_TRADE_ACCEPT;
	else if(tmpStr == "textedit")
		m_type = CREATURE_EVENT_TEXTEDIT;
	else if(tmpStr == "reportbug")
		m_type = CREATURE_EVENT_REPORTBUG;
	else if(tmpStr == "look")
		m_type = CREATURE_EVENT_LOOK;
	else if(tmpStr == "think")
		m_type = CREATURE_EVENT_THINK;
	else if(tmpStr == "direction")
		m_type = CREATURE_EVENT_DIRECTION;
	else if(tmpStr == "outfit")
		m_type = CREATURE_EVENT_OUTFIT;
	else if(tmpStr == "statschange")
		m_type = CREATURE_EVENT_STATSCHANGE;
	else if(tmpStr == "areacombat")
		m_type = CREATURE_EVENT_COMBAT_AREA;
	else if(tmpStr == "push")
		m_type = CREATURE_EVENT_PUSH;
	else if(tmpStr == "target")
		m_type = CREATURE_EVENT_TARGET;
	else if(tmpStr == "follow")
		m_type = CREATURE_EVENT_FOLLOW;
	else if(tmpStr == "combat")
		m_type = CREATURE_EVENT_COMBAT;
	else if(tmpStr == "attack")
		m_type = CREATURE_EVENT_ATTACK;
	else if(tmpStr == "cast")
		m_type = CREATURE_EVENT_CAST;
	else if(tmpStr == "kill")
		m_type = CREATURE_EVENT_KILL;
	else if(tmpStr == "death")
		m_type = CREATURE_EVENT_DEATH;
	else if(tmpStr == "preparedeath")
		m_type = CREATURE_EVENT_PREPAREDEATH;
	#ifdef __DARGHOS_PVP_SYSTEM__
	else if(tmpStr == "bgdeath")
		m_type = CREATURE_EVENT_BG_DEATH;
	else if(tmpStr == "bgend")
		m_type = CREATURE_EVENT_BG_END;
	else if(tmpStr == "bgleave")
		m_type = CREATURE_EVENT_BG_LEAVE;
	#endif
#ifdef __DARGHOS_CUSTOM__
	else if(tmpStr == "moveitem")
		m_type = CREATURE_EVENT_MOVE_ITEM;
#endif
	else
	{
		std::clog << "[Error - CreatureEvent::configureEvent] No valid type for creature event." << str << std::endl;
		return false;
	}

	m_isLoaded = true;
	return true;
}

std::string CreatureEvent::getScriptEventName() const
{
	switch(m_type)
	{
		case CREATURE_EVENT_LOGIN:
			return "onLogin";
		case CREATURE_EVENT_LOGOUT:
			return "onLogout";
		case CREATURE_EVENT_CHANNEL_JOIN:
			return "onJoinChannel";
		case CREATURE_EVENT_CHANNEL_LEAVE:
			return "onLeaveChannel";
		case CREATURE_EVENT_THINK:
			return "onThink";
		case CREATURE_EVENT_ADVANCE:
			return "onAdvance";
		case CREATURE_EVENT_LOOK:
			return "onLook";
		case CREATURE_EVENT_DIRECTION:
			return "onDirection";
		case CREATURE_EVENT_OUTFIT:
			return "onOutfit";
		case CREATURE_EVENT_MAIL_SEND:
			return "onSendMail";
		case CREATURE_EVENT_MAIL_RECEIVE:
			return "onReceiveMail";
		case CREATURE_EVENT_TRADE_REQUEST:
			return "onTradeRequest";
		case CREATURE_EVENT_TRADE_ACCEPT:
			return "onTradeAccept";
		case CREATURE_EVENT_TEXTEDIT:
			return "onTextEdit";
		case CREATURE_EVENT_REPORTBUG:
			return "onReportBug";
		case CREATURE_EVENT_STATSCHANGE:
			return "onStatsChange";
		case CREATURE_EVENT_COMBAT_AREA:
			return "onAreaCombat";
		case CREATURE_EVENT_PUSH:
			return "onPush";
		case CREATURE_EVENT_TARGET:
			return "onTarget";
		case CREATURE_EVENT_FOLLOW:
			return "onFollow";
		case CREATURE_EVENT_COMBAT:
			return "onCombat";
		case CREATURE_EVENT_ATTACK:
			return "onAttack";
		case CREATURE_EVENT_CAST:
			return "onCast";
		case CREATURE_EVENT_KILL:
			return "onKill";
		case CREATURE_EVENT_DEATH:
			return "onDeath";
		case CREATURE_EVENT_PREPAREDEATH:
			return "onPrepareDeath";
		#ifdef __DARGHOS_PVP_SYSTEM__
		case CREATURE_EVENT_BG_DEATH:
			return "onBattlegroundDeath";
		case CREATURE_EVENT_BG_END:
			return "onBattlegroundEnd";
		case CREATURE_EVENT_BG_LEAVE:
			return "onBattlegroundLeave";
		#endif
#ifdef __DARGHOS_CUSTOM__
		case CREATURE_EVENT_MOVE_ITEM:
			return "onMoveItem";
#endif
		case CREATURE_EVENT_NONE:
		default:
			break;
	}

	return "";
}

std::string CreatureEvent::getScriptEventParams() const
{
	switch(m_type)
	{
		case CREATURE_EVENT_LOGIN:
		#ifdef __DARGHOS_PVP_SYSTEM__
		case CREATURE_EVENT_BG_LEAVE:
		#endif
			return "cid";
		case CREATURE_EVENT_LOGOUT:
			return "cid, forceLogout";
		case CREATURE_EVENT_CHANNEL_JOIN:
		case CREATURE_EVENT_CHANNEL_LEAVE:
			return "cid, channel, users";
		case CREATURE_EVENT_ADVANCE:
			return "cid, skill, oldLevel, newLevel";
		case CREATURE_EVENT_LOOK:
			return "cid, thing, position, lookDistance";
		case CREATURE_EVENT_MAIL_SEND:
			return "cid, receiver, item, openBox";
		case CREATURE_EVENT_MAIL_RECEIVE:
			return "cid, sender, item, openBox";
		case CREATURE_EVENT_TRADE_REQUEST:
		case CREATURE_EVENT_TRADE_ACCEPT:
			return "cid, target, item";
		case CREATURE_EVENT_TEXTEDIT:
			return "cid, item, newText";
		case CREATURE_EVENT_REPORTBUG:
			return "cid, comment";
		case CREATURE_EVENT_THINK:
			return "cid, interval";
		case CREATURE_EVENT_DIRECTION:
		case CREATURE_EVENT_OUTFIT:
			return "cid, old, current";
		case CREATURE_EVENT_STATSCHANGE:
			return "cid, attacker, type, combat, value";
		case CREATURE_EVENT_COMBAT_AREA:
			return "cid, ground, position, aggressive";
		case CREATURE_EVENT_PUSH:
		case CREATURE_EVENT_TARGET:
		case CREATURE_EVENT_FOLLOW:
		case CREATURE_EVENT_COMBAT:
		case CREATURE_EVENT_ATTACK:
		case CREATURE_EVENT_CAST:
			return "cid, target";
		case CREATURE_EVENT_KILL:
			return "cid, target, damage, flags, war";
		case CREATURE_EVENT_DEATH:
			return "cid, corpse, deathList";
		case CREATURE_EVENT_PREPAREDEATH:
			return "cid, deathList";
#ifdef __DARGHOS_PVP_SYSTEM__
        case CREATURE_EVENT_BG_END:
            return "cid, winner, timeIn, bgDuration, initIn";
        case CREATURE_EVENT_BG_DEATH:
            return "cid, lastDamager, assistList";
#endif
#ifdef __DARGHOS_CUSTOM__
		case CREATURE_EVENT_MOVE_ITEM:
			return "cid, item, position";
#endif
		case CREATURE_EVENT_NONE:
		default:
			break;
	}

	return "";
}

void CreatureEvent::copyEvent(CreatureEvent* creatureEvent)
{
	m_scriptId = creatureEvent->m_scriptId;
	m_interface = creatureEvent->m_interface;
	m_scripted = creatureEvent->m_scripted;
	m_isLoaded = creatureEvent->m_isLoaded;
}

void CreatureEvent::clearEvent()
{
	m_scriptId = 0;
	m_interface = NULL;
	m_scripted = EVENT_SCRIPT_FALSE;
	m_isLoaded = false;
}

uint32_t CreatureEvent::executeLogin(Player* player)
{
	//onLogin(cid)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());
			std::stringstream scriptstream;
			scriptstream << "local cid = " << env->addThing(player) << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEvent(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);
			lua_pushnumber(L, env->addThing(player));

			bool result = m_interface->callFunction(1);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeLogin] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeLogout(Player* player, bool forceLogout)
{
	//onLogout(cid, forceLogout)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());
			std::stringstream scriptstream;

			scriptstream << "local cid = " << env->addThing(player) << std::endl;
			scriptstream << "local forceLogout = " << (forceLogout ? "true" : "false") << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEvent(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			lua_pushboolean(L, forceLogout);

			bool result = m_interface->callFunction(2);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeLogout] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeChannelJoin(Player* player, uint16_t channelId, UsersMap usersMap)
{
	//onJoinChannel(cid, channel, users)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());
			std::stringstream scriptstream;
			scriptstream << "local cid = " << env->addThing(player) << std::endl;

			scriptstream << "local channel = " << channelId << std::endl;
			scriptstream << "local users = {}" << std::endl;
			for(UsersMap::iterator it = usersMap.begin(); it != usersMap.end(); ++it)
				scriptstream << "users:insert(" << env->addThing(it->second) << ")" << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEvent(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			lua_pushnumber(L, channelId);

			UsersMap::iterator it = usersMap.begin();
			lua_newtable(L);
			for(int32_t i = 1; it != usersMap.end(); ++it, ++i)
			{
				lua_pushnumber(L, i);
				lua_pushnumber(L, env->addThing(it->second));
				lua_settable(L, -3);
			}

			bool result = m_interface->callFunction(3);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeChannelJoin] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeChannelLeave(Player* player, uint16_t channelId, UsersMap usersMap)
{
	//onLeaveChannel(cid, channel, users)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());
			std::stringstream scriptstream;
			scriptstream << "local cid = " << env->addThing(player) << std::endl;

			scriptstream << "local channel = " << channelId << std::endl;
			scriptstream << "local users = {}" << std::endl;
			for(UsersMap::iterator it = usersMap.begin(); it != usersMap.end(); ++it)
				scriptstream << "users:insert(" << env->addThing(it->second) << ")" << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEvent(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			lua_pushnumber(L, channelId);

			UsersMap::iterator it = usersMap.begin();
			lua_newtable(L);
			for(int32_t i = 1; it != usersMap.end(); ++it, ++i)
			{
				lua_pushnumber(L, i);
				lua_pushnumber(L, env->addThing(it->second));
				lua_settable(L, -3);
			}

			bool result = m_interface->callFunction(3);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeChannelLeave] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeAdvance(Player* player, skills_t skill, uint32_t oldLevel, uint32_t newLevel)
{
	//onAdvance(cid, skill, oldLevel, newLevel)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());
			std::stringstream scriptstream;
			scriptstream << "local cid = " << env->addThing(player) << std::endl;

			scriptstream << "local skill = " << skill << std::endl;
			scriptstream << "local oldLevel = " << oldLevel << std::endl;
			scriptstream << "local newLevel = " << newLevel << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEvent(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			lua_pushnumber(L, (uint32_t)skill);

			lua_pushnumber(L, oldLevel);
			lua_pushnumber(L, newLevel);

			bool result = m_interface->callFunction(4);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeAdvance] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeMailSend(Player* player, Player* receiver, Item* item, bool openBox)
{
	//onSendMail(cid, receiver, item, openBox)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());
			std::stringstream scriptstream;
			scriptstream << "local cid = " << env->addThing(player) << std::endl;

			scriptstream << "local receiver = " << env->addThing(receiver) << std::endl;
			env->streamThing(scriptstream, "item", item, env->addThing(item));
			scriptstream << "local openBox = " << (openBox ? "true" : "false") << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[30];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEvent(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			lua_pushnumber(L, env->addThing(receiver));

			LuaInterface::pushThing(L, item, env->addThing(item));
			lua_pushboolean(L, openBox);

			bool result = m_interface->callFunction(4);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeMailSend] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeMailReceive(Player* player, Player* sender, Item* item, bool openBox)
{
	//onReceiveMail(cid, sender, item, openBox)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());
			std::stringstream scriptstream;
			scriptstream << "local cid = " << env->addThing(player) << std::endl;

			scriptstream << "local sender = " << env->addThing(sender) << std::endl;
			env->streamThing(scriptstream, "item", item, env->addThing(item));
			scriptstream << "local openBox = " << (openBox ? "true" : "false") << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[30];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEvent(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			lua_pushnumber(L, env->addThing(sender));

			LuaInterface::pushThing(L, item, env->addThing(item));
			lua_pushboolean(L, openBox);

			bool result = m_interface->callFunction(4);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeMailReceive] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeTradeRequest(Player* player, Player* target, Item* item)
{
	//onTradeRequest(cid, target, item)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());
			std::stringstream scriptstream;
			scriptstream << "local cid = " << env->addThing(player) << std::endl;

			scriptstream << "local target = " << env->addThing(target) << std::endl;
			env->streamThing(scriptstream, "item", item, env->addThing(item));

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEvent(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			lua_pushnumber(L, env->addThing(target));
			LuaInterface::pushThing(L, item, env->addThing(item));

			bool result = m_interface->callFunction(3);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeTradeRequest] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeTradeAccept(Player* player, Player* target, Item* item, Item* targetItem)
{
	//onTradeAccept(cid, target, item, targetItem)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());
			std::stringstream scriptstream;
			scriptstream << "local cid = " << env->addThing(player) << std::endl;

			scriptstream << "local target = " << env->addThing(target) << std::endl;
			env->streamThing(scriptstream, "item", item, env->addThing(item));

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEvent(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			lua_pushnumber(L, env->addThing(target));
			LuaInterface::pushThing(L, item, env->addThing(item));
			LuaInterface::pushThing(L, targetItem, env->addThing(targetItem));

			bool result = m_interface->callFunction(4);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeTradeAccept] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeLook(Player* player, Thing* thing, const Position& position, int16_t stackpos, int32_t lookDistance)
{
	//onLook(cid, thing, position, lookDistance)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());
			std::stringstream scriptstream;
			scriptstream << "local cid = " << env->addThing(player) << std::endl;

			scriptstream << "local thing = " << env->addThing(thing) << std::endl;
			env->streamPosition(scriptstream, "position", position, stackpos);
			scriptstream << "local lookDistance = " << lookDistance << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[30];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEvent(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			LuaInterface::pushThing(L, thing, env->addThing(thing));

			LuaInterface::pushPosition(L, position, stackpos);
			lua_pushnumber(L, lookDistance);

			bool result = m_interface->callFunction(4);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeLook] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeDirection(Creature* creature, Direction old, Direction current)
{
	//onDirection(cid, old, current)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());
			std::stringstream scriptstream;
			scriptstream << "local cid = " << env->addThing(creature) << std::endl;

			scriptstream << "local old = " << old << std::endl;
			scriptstream << "local current = " << current << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[30];
			sprintf(desc, "%s", creature->getName().c_str());
			env->setEvent(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			lua_pushnumber(L, old);
			lua_pushnumber(L, current);

			bool result = m_interface->callFunction(3);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeDirection] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOutfit(Creature* creature, const Outfit_t& old, const Outfit_t& current)
{
	//onOutfit(cid, old, current)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());
			std::stringstream scriptstream;
			scriptstream << "local cid = " << env->addThing(creature) << std::endl;

			env->streamOutfit(scriptstream, "old", old);
			env->streamOutfit(scriptstream, "current", current);

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[30];
			sprintf(desc, "%s", creature->getName().c_str());
			env->setEvent(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			LuaInterface::pushOutfit(L, old);
			LuaInterface::pushOutfit(L, current);

			bool result = m_interface->callFunction(3);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeOutfit] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeThink(Creature* creature, uint32_t interval)
{
	//onThink(cid, interval)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());
			std::stringstream scriptstream;

			scriptstream << "local cid = " << env->addThing(creature) << std::endl;
			scriptstream << "local interval = " << interval << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", creature->getName().c_str());
			env->setEvent(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			lua_pushnumber(L, interval);

			bool result = m_interface->callFunction(2);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeThink] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeStatsChange(Creature* creature, Creature* attacker, StatsChange_t type, CombatType_t combat, int32_t value)
{
	//onStatsChange(cid, attacker, type, combat, value)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());
			std::stringstream scriptstream;

			scriptstream << "local cid = " << env->addThing(creature) << std::endl;
			scriptstream << "local attacker = " << env->addThing(attacker) << std::endl;

			scriptstream << "local type = " << (uint32_t)type << std::endl;
			scriptstream << "local combat = " << (uint32_t)combat << std::endl;
			scriptstream << "local value = " << value << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", creature->getName().c_str());
			env->setEvent(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			lua_pushnumber(L, env->addThing(attacker));

			lua_pushnumber(L, (uint32_t)type);
			lua_pushnumber(L, (uint32_t)combat);
			lua_pushnumber(L, value);

			bool result = m_interface->callFunction(5);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeStatsChange] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeCombatArea(Creature* creature, Tile* tile, bool aggressive)
{
	//onAreaCombat(cid, ground, position, aggressive)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());
			std::stringstream scriptstream;
			scriptstream << "local cid = " << env->addThing(creature) << std::endl;

			env->streamThing(scriptstream, "ground", tile->ground, env->addThing(tile->ground));
			env->streamPosition(scriptstream, "position", tile->getPosition(), 0);
			scriptstream << "local aggressive = " << (aggressive ? "true" : "false") << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			std::stringstream desc;
			desc << creature->getName();
			env->setEvent(desc.str());
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			LuaInterface::pushThing(L, tile->ground, env->addThing(tile->ground));

			LuaInterface::pushPosition(L, tile->getPosition(), 0);
			lua_pushboolean(L, aggressive);

			bool result = m_interface->callFunction(4);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeAreaCombat] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeCombat(Creature* creature, Creature* target)
{
	//onCombat(cid, target)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());
			std::stringstream scriptstream;

			scriptstream << "local cid = " << env->addThing(creature) << std::endl;
			scriptstream << "local target = " << env->addThing(target) << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			std::stringstream desc;
			desc << creature->getName();
			env->setEvent(desc.str());
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			lua_pushnumber(L, env->addThing(target));

			bool result = m_interface->callFunction(2);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeCombat] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeAttack(Creature* creature, Creature* target)
{
	//onAttack(cid, target)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());
			std::stringstream scriptstream;

			scriptstream << "local cid = " << env->addThing(creature) << std::endl;
			scriptstream << "local target = " << env->addThing(target) << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			std::stringstream desc;
			desc << creature->getName();
			env->setEvent(desc.str());
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			lua_pushnumber(L, env->addThing(target));

			bool result = m_interface->callFunction(2);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeAttack] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeCast(Creature* creature, Creature* target/* = NULL*/)
{
	//onCast(cid[, target])
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());
			std::stringstream scriptstream;

			scriptstream << "local cid = " << env->addThing(creature) << std::endl;
			scriptstream << "local target = ";
			if(target)
				scriptstream << env->addThing(target);
			else
				scriptstream << "nil";

			scriptstream << std::endl << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			std::stringstream desc;
			desc << creature->getName();
			env->setEvent(desc.str());
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			lua_pushnumber(L, env->addThing(target));

			bool result = m_interface->callFunction(2);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeCast] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeKill(Creature* creature, Creature* target, const DeathEntry& entry)
{
	//onKill(cid, target, damage, flags)
	if(m_interface->reserveEnv())
	{
		uint32_t flags = 0;
		if(entry.isLast())
			flags |= 1;

		if(entry.isJustify())
			flags |= 2;

		if(entry.isUnjustified())
			flags |= 4;

		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());
			std::stringstream scriptstream;
			scriptstream << "local cid = " << env->addThing(creature) << std::endl;

			scriptstream << "local target = " << env->addThing(target) << std::endl;
			scriptstream << "local damage = " << entry.getDamage() << std::endl;
			scriptstream << "local flags = " << flags << std::endl;
			scriptstream << "local war = " << entry.getWar().war << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			std::stringstream desc;
			desc << creature->getName();
			env->setEvent(desc.str());
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			lua_pushnumber(L, env->addThing(target));

			lua_pushnumber(L, entry.getDamage());
			lua_pushnumber(L, flags);
			lua_pushnumber(L, entry.getWar().war);

			bool result = m_interface->callFunction(5);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeKill] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeDeath(Creature* creature, Item* corpse, DeathList deathList)
{
	//onDeath(cid, corpse, deathList)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());
			std::stringstream scriptstream;
			scriptstream << "local cid = " << env->addThing(creature) << std::endl;

			env->streamThing(scriptstream, "corpse", corpse, env->addThing(corpse));
			scriptstream << "local deathList = {}" << std::endl;
			for(DeathList::iterator it = deathList.begin(); it != deathList.end(); ++it)
			{
				scriptstream << "deathList:insert(";
				if(it->isCreatureKill())
					scriptstream << env->addThing(it->getKillerCreature());
				else
					scriptstream << it->getKillerName();

				scriptstream << ")" << std::endl;
			}

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", creature->getName().c_str());
			env->setEvent(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			LuaInterface::pushThing(L, corpse, env->addThing(corpse));

			lua_newtable(L);
			DeathList::iterator it = deathList.begin();
			for(int32_t i = 1; it != deathList.end(); ++it, ++i)
			{
				lua_pushnumber(L, i);
				if(it->isCreatureKill())
					lua_pushnumber(L, env->addThing(it->getKillerCreature()));
				else
					lua_pushstring(L, it->getKillerName().c_str());

				lua_settable(L, -3);
			}

			bool result = m_interface->callFunction(3);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeDeath] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executePrepareDeath(Creature* creature, DeathList deathList)
{
	//onPrepareDeath(cid, deathList)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());
			std::stringstream scriptstream;
			scriptstream << "local cid = " << env->addThing(creature) << std::endl;

			scriptstream << "local deathList = {}" << std::endl;
			for(DeathList::iterator it = deathList.begin(); it != deathList.end(); ++it)
			{
				scriptstream << "deathList:insert(";
				if(it->isCreatureKill())
					scriptstream << env->addThing(it->getKillerCreature());
				else
					scriptstream << it->getKillerName();

				scriptstream << ")" << std::endl;
			}

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", creature->getName().c_str());
			env->setEvent(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));

			lua_newtable(L);
			DeathList::iterator it = deathList.begin();
			for(int32_t i = 1; it != deathList.end(); ++it, ++i)
			{
				lua_pushnumber(L, i);
				if(it->isCreatureKill())
					lua_pushnumber(L, env->addThing(it->getKillerCreature()));
				else
					lua_pushstring(L, it->getKillerName().c_str());

				lua_settable(L, -3);
			}

			bool result = m_interface->callFunction(2);
			m_interface->releaseEnv();

			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executePrepareDeath] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeTextEdit(Player* player, Item* item, std::string newText)
{
	//onTextEdit(cid, item, newText)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());
			std::stringstream scriptstream;
			scriptstream << "local cid = " << env->addThing(player) << std::endl;

			env->streamThing(scriptstream, "item", item, env->addThing(item));
			scriptstream << "local newText = " << newText.c_str() << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEvent(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			LuaInterface::pushThing(L, item, env->addThing(item));
			lua_pushstring(L, newText.c_str());

			bool result = m_interface->callFunction(3);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeTextEdit] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeReportBug(Player* player, std::string comment)
{
	//onReportBug(cid, comment)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());
			std::stringstream scriptstream;

			scriptstream << "local cid = " << env->addThing(player) << std::endl;
			scriptstream << "local comment = " << comment.c_str() << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEvent(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			lua_pushstring(L, comment.c_str());

			bool result = m_interface->callFunction(2);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeReportBug] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executePush(Player* player, Creature* target)
{
	//onPush(cid, target)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());
			std::stringstream scriptstream;

			scriptstream << "local cid = " << env->addThing(player) << std::endl;
			scriptstream << "local target = " << env->addThing(target) << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			std::stringstream desc;
			desc << player->getName();
			env->setEvent(desc.str());
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			lua_pushnumber(L, env->addThing(target));

			bool result = m_interface->callFunction(2);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executePush] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeTarget(Creature* creature, Creature* target)
{
	//onTarget(cid, target)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());
			std::stringstream scriptstream;

			scriptstream << "local cid = " << env->addThing(creature) << std::endl;
			scriptstream << "local target = " << env->addThing(target) << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			std::stringstream desc;
			desc << creature->getName();
			env->setEvent(desc.str());
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			lua_pushnumber(L, env->addThing(target));

			bool result = m_interface->callFunction(2);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeTarget] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeFollow(Creature* creature, Creature* target)
{
	//onFollow(cid, target)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());
			std::stringstream scriptstream;

			scriptstream << "local cid = " << env->addThing(creature) << std::endl;
			scriptstream << "local target = " << env->addThing(target) << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			std::stringstream desc;
			desc << creature->getName();
			env->setEvent(desc.str());
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			lua_pushnumber(L, env->addThing(target));

			bool result = m_interface->callFunction(2);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeFollow] Call stack overflow." << std::endl;
		return 0;
	}
}

#ifdef __DARGHOS_PVP_SYSTEM__
uint32_t CreatureEvent::executeBgDeath(Player* player, Player* lastDamager, std::list<uint32_t> assistList)
{
	//onBattlegroundDeath(cid, lastDamager, assistList)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());
			std::stringstream scriptstream;

			scriptstream << "local cid = " << env->addThing(player) << std::endl;
			scriptstream << "local lastDamager = " << env->addThing(lastDamager) << std::endl;

			scriptstream << "local assistList = {}" << std::endl;
			std::list<uint32_t>::iterator it;
			for(it = assistList.begin(); it != assistList.end(); ++it)
			{
				scriptstream << "assistList:insert(" << (*it) << ")";
			}

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			std::stringstream desc;
			desc << creature->getName();
			env->setEvent(desc.str());
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			lua_pushnumber(L, env->addThing(lastDamager));

			lua_newtable(L);
			std::list<uint32_t>::iterator it = assistList.begin();
			for(int32_t i = 1;it != assistList.end(); ++it, i++)
			{
			    lua_pushnumber(L, i);
			    lua_pushnumber(L, *it);

				lua_settable(L, -3);
			}

			bool result = m_interface->callFunction(3);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeFollow] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeBgEnd(Player* player, bool isWinner, uint32_t timeIn, uint32_t bgDuration, uint32_t initIn)
{
	//onBattlegroundEnd(cid, winner, timeIn, bgDuration, initIn)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());
			std::stringstream scriptstream;

			scriptstream << "local cid = " << env->addThing(player) << std::endl;
			scriptstream << "local winner = " << (isWinner ? "true" : "false") << std::endl;
			scriptstream << "local timeIn = " << timeIn << std::endl;
			scriptstream << "local bgDuration = " << bgDuration << std::endl;
			scriptstream << "local initIn = " << initIn << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			std::stringstream desc;
			desc << player->getName();
			env->setEvent(desc.str());
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			lua_pushboolean(L, isWinner);
			lua_pushnumber(L, timeIn);
			lua_pushnumber(L, bgDuration);
			lua_pushnumber(L, initIn);

			bool result = m_interface->callFunction(5);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeFollow] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeBgLeave(Player* player)
{
	//onBattlegroundLeave(cid)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());
			std::stringstream scriptstream;

			scriptstream << "local cid = " << env->addThing(player) << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			std::stringstream desc;
			desc << creature->getName();
			env->setEvent(desc.str());
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));

			bool result = m_interface->callFunction(1);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executeFollow] Call stack overflow." << std::endl;
		return 0;
	}
}
#endif

#ifdef __DARGHOS_CUSTOM__
uint32_t CreatureEvent::executeMoveItem(Player* player, Item* item, const Position &position)
{
	//onMoveItem(cid, item, position)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());
			std::stringstream scriptstream;

			scriptstream << "local cid = " << env->addThing(player) << std::endl;
			env->streamThing(scriptstream, "item", item, env->addThing(item));
			env->streamPosition(scriptstream, "position", position, 0);

			scriptstream << m_scriptData;
			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			std::stringstream desc;
			desc << player->getName();
			env->setEvent(desc.str());
			#endif

			env->setScriptId(m_scriptId, m_interface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			LuaInterface::pushThing(L, item, env->addThing(item));
			LuaInterface::pushPosition(L, position, 0);

			bool result = m_interface->callFunction(3);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - CreatureEvent::executePush] Call stack overflow." << std::endl;
		return 0;
	}
}
#endif
