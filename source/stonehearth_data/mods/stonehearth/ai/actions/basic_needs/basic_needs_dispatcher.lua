local BasicNeedsDispatcher = class()
BasicNeedsDispatcher.name = 'basic needs'
BasicNeedsDispatcher.does = 'stonehearth:top'
BasicNeedsDispatcher.args = {}
BasicNeedsDispatcher.version = 2
BasicNeedsDispatcher.priority = stonehearth.constants.priorities.top.BASIC_NEEDS

local ai = stonehearth.ai
return ai:create_compound_action(BasicNeedsDispatcher)
         :execute('stonehearth:basic_needs')
