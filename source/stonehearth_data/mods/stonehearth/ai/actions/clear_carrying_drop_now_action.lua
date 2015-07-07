local ClearCarryingDropNow = class()

ClearCarryingDropNow.name = 'drop carrying onto ground'
ClearCarryingDropNow.does = 'stonehearth:clear_carrying_now'
ClearCarryingDropNow.args = {
}
ClearCarryingDropNow.version = 2
ClearCarryingDropNow.priority = 1 -- run a little lower than put in backpack; this is preferred.

local ai = stonehearth.ai
return ai:create_compound_action(ClearCarryingDropNow)
   :execute('stonehearth:drop_carrying_now', {})
