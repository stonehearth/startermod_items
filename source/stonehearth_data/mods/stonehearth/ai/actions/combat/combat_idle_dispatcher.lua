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
         -- find_target will perform duplicate work when attack action is selected
         -- find an alternative if this gets expensive
         -- find target is usually O(n) per entity, so combat is O(n^2) on number of participants
         :execute('stonehearth:combat:find_target')
         :execute('stonehearth:combat:idle', { enemy = ai.PREV.target })
