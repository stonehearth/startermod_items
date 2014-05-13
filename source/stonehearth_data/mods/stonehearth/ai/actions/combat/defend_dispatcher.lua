local constants = require 'constants'
local Entity = _radiant.om.Entity

local DefendDispatcher = class()
DefendDispatcher.name = 'defend dispatcher'
DefendDispatcher.does = 'stonehearth:combat'
DefendDispatcher.args = {
   enemy = Entity
}
DefendDispatcher.version = 2
DefendDispatcher.priority = constants.priorities.combat.ACTIVE

local ai = stonehearth.ai
return ai:create_compound_action(DefendDispatcher)
         :execute('stonehearth:combat:defend')
