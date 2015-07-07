local ClearCarryingIntoBackpack = class()

ClearCarryingIntoBackpack.name = 'put carrying into backpack'
ClearCarryingIntoBackpack.does = 'stonehearth:clear_carrying_now'
ClearCarryingIntoBackpack.args = {
}
ClearCarryingIntoBackpack.version = 2
ClearCarryingIntoBackpack.priority = 2 -- run a little higher than drop on ground; this is preferred.

local ai = stonehearth.ai
return ai:create_compound_action(ClearCarryingIntoBackpack)
   :execute('stonehearth:put_carrying_in_backpack', {})
