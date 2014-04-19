local FreeTimeDispatcher = class()
FreeTimeDispatcher.name = 'free time'
FreeTimeDispatcher.does = 'stonehearth:top'
FreeTimeDispatcher.args = {}
FreeTimeDispatcher.version = 2
FreeTimeDispatcher.priority = stonehearth.constants.priorities.top.FREE_TIME

local ai = stonehearth.ai
return ai:create_compound_action(FreeTimeDispatcher)
   :execute('stonehearth:free_time')