local constants = require 'constants'
local Entity = _radiant.om.Entity

local CombatIdleDispatcher = class()
CombatIdleDispatcher.name = 'combat idle dispatcher'
CombatIdleDispatcher.does = 'stonehearth:combat'
CombatIdleDispatcher.args = {}
CombatIdleDispatcher.version = 2
CombatIdleDispatcher.priority = constants.priorities.combat.IDLE

local ai = stonehearth.ai
return ai:create_compound_action(CombatIdleDispatcher)
         :execute('stonehearth:combat:get_primary_target')
         :execute('stonehearth:combat:idle', { enemy = ai.PREV.target })
