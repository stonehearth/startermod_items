local UrgentActionsDispatcher = class()
UrgentActionsDispatcher.name = 'urgent actions'
UrgentActionsDispatcher.does = 'stonehearth:top'
UrgentActionsDispatcher.args = {}
UrgentActionsDispatcher.version = 2
UrgentActionsDispatcher.priority = stonehearth.constants.priorities.top.URGENT_ACTIONS

local ai = stonehearth.ai
return ai:create_compound_action(UrgentActionsDispatcher)
         :execute('stonehearth:urgent_actions')
