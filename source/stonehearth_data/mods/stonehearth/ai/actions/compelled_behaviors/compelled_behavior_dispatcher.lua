local CompelledBehaviorDispatcher = class()
CompelledBehaviorDispatcher.name = 'compel entity behavior'
CompelledBehaviorDispatcher.does = 'stonehearth:top'
CompelledBehaviorDispatcher.args = {}
CompelledBehaviorDispatcher.version = 2
CompelledBehaviorDispatcher.priority = stonehearth.constants.priorities.top.COMPELLED_BEHAVIOR

local ai = stonehearth.ai
return ai:create_compound_action(CompelledBehaviorDispatcher)
         :execute('stonehearth:compelled_behavior')
