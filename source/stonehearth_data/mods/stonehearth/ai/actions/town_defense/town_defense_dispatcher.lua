local constants = require 'constants'

local TownDefenseDispatcher = class()
TownDefenseDispatcher.name = 'town defense dispatcher'
TownDefenseDispatcher.does = 'stonehearth:town_defense:dispatcher'
TownDefenseDispatcher.args = {}
TownDefenseDispatcher.version = 2
TownDefenseDispatcher.priority = constants.priorities.urgent_actions.TOWN_DEFENSE

local ai = stonehearth.ai
return ai:create_compound_action(TownDefenseDispatcher)
         :execute('stonehearth:drop_carrying_now')
         :execute('stonehearth:set_posture', { posture = 'stonehearth:combat' })
         :execute('stonehearth:town_defense')
         :execute('stonehearth:unset_posture', { posture = 'stonehearth:combat' })
