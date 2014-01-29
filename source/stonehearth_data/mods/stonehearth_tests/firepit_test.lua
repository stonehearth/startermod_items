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

   self:place_item_cluster('stonehearth:oak_log', -10, 0, 3, 3)

   local faction = radiant.entities.get_faction(worker)
   self:place_item('stonehearth:firepit_proxy', 1, 1, faction)
   local firepit = self:place_item('stonehearth:firepit', 8, 8, faction)
   radiant.events.trigger(stonehearth.calendar, 'stonehearth:hourly')
   --firepit:get_component('stonehearth:firepit'):_init_gather_wood_task()
   self:at(20000,  function()
         radiant.entities.destroy_entity(firepit)
      end)
   
end

return FirePitTest

