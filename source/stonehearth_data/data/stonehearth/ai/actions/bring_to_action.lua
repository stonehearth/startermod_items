--[[
   Use this action to command an entity to go to an item, pick it up,
   and then bring it somewhere to put it down.
]]
local BringToAction = class()

BringToAction.name = 'stonehearth.actions.bring_to'
BringToAction.does = 'stonehearth.activities.bring_to'
BringToAction.priority = 5

function BringToAction:run(ai, entity, item, destination)
   ai:execute('stonehearth.activities.pickup_item', item)
   --TODO: calculate adjacent with pathfinder (this is a fake adjacent right now)
   ai:execute('stonehearth.activities.goto_location', RadiantIPoint3(destination.x, destination.y, destination.z+1))
   ai:execute('stonehearth.activities.drop_carrying', destination)
end

return BringToAction