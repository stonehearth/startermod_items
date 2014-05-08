local constants = require 'constants'

local SleepAction = class()
SleepAction.name = 'goto sleep'
SleepAction.does = 'stonehearth:top'
SleepAction.status_text = 'sleeping'
SleepAction.args = { }
SleepAction.version = 2
SleepAction.priority = constants.priorities.top.SLEEP

local ai = stonehearth.ai
return ai:create_compound_action(SleepAction)
         :execute('stonehearth:sleep')
