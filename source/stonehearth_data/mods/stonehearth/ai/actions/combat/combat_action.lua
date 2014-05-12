local constants = require 'constants'
local log = radiant.log.create_logger('combat')

local Combat = class()
Combat.name = 'combat'
Combat.does = 'stonehearth:top'
Combat.args = {}
Combat.version = 2
Combat.priority = constants.priorities.top.COMBAT

local ai = stonehearth.ai
return ai:create_compound_action(Combat)
   :execute('stonehearth:combat:find_target')
   :execute('stonehearth:drop_carrying_now')
   :execute('stonehearth:set_posture', { posture = 'combat' })
   :execute('stonehearth:combat', { enemy = ai.BACK(3).target })
   :execute('stonehearth:unset_posture', { posture = 'combat' })
