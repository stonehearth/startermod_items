local MicroWorld = require 'lib.micro_world'

local Point3 = _radiant.csg.Point3
local CarryTest = class(MicroWorld)

--[[
   Instantiate a dude and have him pick up and move a series of items
]]


function CarryTest:__init()
   self[MicroWorld]:__init()
   self:create_world()
   local dude = self:place_citizen(12, 12)

   local saw = self:place_item('stonehearth:carpenter_saw', 11, 11)
   local bolt = self:place_item('stonehearth:cloth_bolt', 11, 10)
   local buckler = self:place_item('stonehearth:wooden_buckler', 11, 9)
   local sword = self:place_item('stonehearth:wooden_sword', 11, 8)
   local log =  self:place_item('stonehearth:oak_log', 11, 7)

   local movable_items = {saw, bolt, buckler, sword, log}
   self:at(1000, function()
      -- xxx, this is borked with the new event system.
      --[[
      radiant.events.broadcast_msg('stonehearth:compulsion_event', function(ai, entity)
         local y = -11
         for i, obj in ipairs(movable_items) do
            local target = Point3(-11 , 1, y)
            ai:execute('stonehearth:pickup_item', obj)
            ai:execute('stonehearth:goto_location', Point3(target.x, target.y, target.z+1))
            ai:execute('stonehearth:drop_carrying', target)
            y = y + 1
         end
      end)
      ]]
   end)

end

return CarryTest