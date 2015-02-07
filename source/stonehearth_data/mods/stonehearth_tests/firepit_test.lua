local Point3 = _radiant.csg.Point3
local MicroWorld = require 'lib.micro_world'
local FirePitTest = class(MicroWorld)

function FirePitTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local tree = self:place_tree(-12, -12)
   local tree = self:place_tree(0, -12)
   local worker = self:place_citizen(12, 12)
   --local worker2 = self:place_citizen(11, 12)

   self:place_item_cluster('stonehearth:resources:wood:oak_log', -10, 0, 3, 3)

   local player_id = radiant.entities.get_player_id(worker)
   self:place_item('stonehearth:decoration:firepit', 1, 1, player_id, { force_iconic = false })
   local firepit = self:place_item('stonehearth:decoration:firepit', 8, 8, player_id)
   self:at(1000,  function()
         --radiant.entities.destroy_entity(firepit)
         stonehearth.calendar:set_time_unit_test_only({ hour = 24, minute = 38 })
      end)   
end

return FirePitTest

