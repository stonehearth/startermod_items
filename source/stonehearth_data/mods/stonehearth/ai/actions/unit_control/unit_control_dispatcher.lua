local PlayerCommandDispatcher = class()
PlayerCommandDispatcher.name = 'follow player commands'
PlayerCommandDispatcher.does = 'stonehearth:top'
PlayerCommandDispatcher.args = {}
PlayerCommandDispatcher.version = 2
PlayerCommandDispatcher.priority = stonehearth.constants.priorities.top.UNIT_CONTROL

local ai = stonehearth.ai
return ai:create_compound_action(PlayerCommandDispatcher)
         :execute('stonehearth:unit_control')
