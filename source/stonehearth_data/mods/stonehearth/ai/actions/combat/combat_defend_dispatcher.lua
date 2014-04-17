local constants = require 'constants'
local Entity = _radiant.om.Entity

local CombatDefendDispatcher = class()
CombatDefendDispatcher.name = 'combat defend dispatcher'
CombatDefendDispatcher.does = 'stonehearth:combat'
CombatDefendDispatcher.args = {
   enemy = Entity
}
CombatDefendDispatcher.version = 2
CombatDefendDispatcher.priority = constants.priorities.combat.ACTIVE

local ai = stonehearth.ai
return ai:create_compound_action(CombatDefendDispatcher)
         :execute('stonehearth:combat:defend')
