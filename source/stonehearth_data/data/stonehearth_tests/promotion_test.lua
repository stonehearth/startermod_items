local MicroWorld = require 'stonehearth_tests.lib.micro_world'

local PromoteTest = class(MicroWorld)
--[[
   Instantiate a carpenter, a workbench, and a piece of wood.
   Turn out a wooden sword
]]

function PromoteTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   --Create the carpenter, bench, and instantiate them to each other

   local worker = self:place_citizen(12, 12)

   local bench = self:place_item('/stonehearth_carpenter_class/entities/carpenter_workbench', -12, -12)
   local workshop_component = bench:get_component('stonehearth_crafter:workshop')
   local faction = worker:get_component('unit_info'):get_faction()
   bench:add_component('unit_info'):set_faction(faction)


   local saw = radiant.entities.create_entity('/stonehearth_items/carpenter_saw')
   workshop_component:set_saw_entity(saw)

   local tree = self:place_tree(-12, 0)
   local tree2 = self:place_tree(-12, 12)

   saw:add_component('unit_info'):set_faction(faction)
   saw:get_component('stonehearth_classes:profession_info'):set_workshop(workshop_component)

   --Human TODO
   --Test that the dude can chop down the tree
   --Test that the dude can be promoted to carpenter
   --Test that he can craft a sword
   --Test that he can no longer chop down the tree

end

return PromoteTest

