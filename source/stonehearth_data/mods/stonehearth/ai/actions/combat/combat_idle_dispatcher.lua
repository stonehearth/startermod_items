local constants = require 'constants'
local Entity = _radiant.om.Entity

local CombatIdleDispatcher = class()
CombatIdleDispatcher.name = 'combat idle dispatcher'
CombatIdleDispatcher.does = 'stonehearth:combat'
CombatIdleDispatcher.args = {}
CombatIdleDispatcher.version = 2
CombatIdleDispatcher.priority = constants.priorities.combat.IDLE
CombatIdleDispatcher.realtime = true

local ai = stonehearth.ai
return ai:create_compound_action(CombatIdleDispatcher)
         :execute('stonehearth:combat:get_primary_target')
         -- don't idle if there is no path to attack the entity
         -- i.e. looks stupid for entities to be in combat when separated by a wall or cliff
         :execute('stonehearth:find_path_to_entity', { destination = ai.PREV.target })
         :execute('stonehearth:combat:idle', { enemy = ai.BACK(2).target })
