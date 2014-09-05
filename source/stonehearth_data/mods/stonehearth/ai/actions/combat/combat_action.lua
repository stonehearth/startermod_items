local constants = require 'constants'
local log = radiant.log.create_logger('combat')

local Combat = class()
Combat.name = 'combat'
Combat.status_text = 'engaged in combat'
Combat.does = 'stonehearth:top'
Combat.args = {}
Combat.version = 2
Combat.priority = constants.priorities.top.COMBAT

local ai = stonehearth.ai
return ai:create_compound_action(Combat)
   :execute('stonehearth:drop_carrying_now', {drop_always = true})
   :execute('stonehearth:set_posture', { posture = 'stonehearth:combat' })
   :execute('stonehearth:combat')
