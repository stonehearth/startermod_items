local constants = require('constants')

local SleepAction = class()
SleepAction.name = 'goto sleep'
SleepAction.does = 'stonehearth:top'
SleepAction.args = { }
SleepAction.version = 2
SleepAction.priority = constants.priorities.needs.SLEEP

local ai = stonehearth.ai
return ai:create_compound_action(SleepAction)
         :execute('stonehearth:add_buff', { buff = 'stonehearth:buffs:sleeping' })
         :execute('stonehearth:sleep')
