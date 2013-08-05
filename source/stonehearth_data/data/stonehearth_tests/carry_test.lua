local MicroWorld = require 'stonehearth_tests.lib.micro_world'

local RadiantIPoint3 = _radiant.math3d.RadiantIPoint3
local CarryTest = class(MicroWorld)

--[[
   Instantiate a dude and have him pick up and move a series of items
]]


function CarryTest:__init()
   self[MicroWorld]:__init()
   self:create_world()
   local dude = self:place_citizen(12, 12)

   local saw = self:place_item('/stonehearth_items/carpenter_saw', 11, 11)
   local bolt = self:place_item('/stonehearth_items/cloth_bolt', 11, 10)
   local buckler = self:place_item('/stonehearth_items/wooden_buckler', 11, 9)
   local sword = self:place_item('/stonehearth_items/wooden_sword', 11, 8)
   local log =  self:place_item('/stonehearth_trees/entities/oak_tree/oak_log', 11, 7)

   local movable_items = {saw, bolt, buckler, sword, log}
   self:at(1000, function()
      radiant.events.broadcast_msg('stonehearth.events.compulsion_event', function(ai, entity)
         local y = -11
         for i, obj in ipairs(movable_items) do
            local target = RadiantIPoint3(-11 , 1, y)
            ai:execute('stonehearth.activities.pickup_item', obj)
            ai:execute('stonehearth.activities.goto_location', RadiantIPoint3(target.x, target.y, target.z+1))
            ai:execute('stonehearth.activities.drop_carrying', target)
            y = y + 1
         end
      end)
   end)

end

return CarryTest