-- @title stonehearth:drop_carrying_at
-- @book reference
-- @section activities

local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity

--[[ @markdown
Use stonehearth:drop\_carrying\_at when you want to drop an item at a certain location. 

For example,  _place\_item_, takes a target location from the user and then calls ai:execute('stonehearth:drop\_carrying\_at') once the item has been picked up. 

stonehearth:drop\_carrying\_at is only implemented by the drop\_carrying\_at\_action.lua file: 
]]--


local DropCarrying = class()
DropCarrying.name = 'drop carrying'
DropCarrying.does = 'stonehearth:drop_carrying_at'
DropCarrying.args = {
   location = Point3      -- where to drop it
}
DropCarrying.think_output = {
   item = Entity,          -- what got dropped
}
DropCarrying.version = 2
DropCarrying.priority = 1


local ai = stonehearth.ai
return ai:create_compound_action(DropCarrying)
         :execute('stonehearth:goto_location', {
            reason = 'drop carrying at',
            location = ai.ARGS.location,
            stop_when_adjacent = true,
         })
         :execute('stonehearth:drop_carrying_adjacent', {
            location = ai.ARGS.location
         })
         :set_think_output({
            item = ai.PREV.item
         })
