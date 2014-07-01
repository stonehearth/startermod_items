--[[
   Death is always on top. If we notice that our HP is <= 0, do "stonehearth:die".
   Whichever death action has highest priority (is best suited) will play
]]

local DeathAction = class()

DeathAction.name = 'die'
DeathAction.does = 'stonehearth:top'
DeathAction.status_text = 'dead'
DeathAction.args = {}
DeathAction.version = 2
DeathAction.priority = stonehearth.constants.priorities.top.DIE

local ai = stonehearth.ai
return ai:create_compound_action(DeathAction)
   :execute('stonehearth:wait_for_attribute_below', {attribute = 'health', value = 1})
   :execute('stonehearth:die')
