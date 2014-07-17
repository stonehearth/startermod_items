local constants = require 'constants'

local TownDefenseIdleDispatcher = class()
TownDefenseIdleDispatcher.name = 'town defense idle dispatcher'
TownDefenseIdleDispatcher.does = 'stonehearth:town_defense'
TownDefenseIdleDispatcher.args = {}
TownDefenseIdleDispatcher.version = 2
TownDefenseIdleDispatcher.priority = constants.priorities.emergencies.TOWN_DEFENSE_IDLE

local ai = stonehearth.ai
return ai:create_compound_action(TownDefenseIdleDispatcher)
   :execute('stonehearth:town_defense:idle')
