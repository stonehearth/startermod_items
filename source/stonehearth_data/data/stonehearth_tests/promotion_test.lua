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

   saw:add_component('unit_info'):set_faction(faction)
   saw:get_component('stonehearth_classes:profession_info'):set_workshop(workshop_component)

   --self:at(5000, function()
   --   radiant.events.broadcast_msg('stonehearth_item.promote_citizen', {talisman = saw, target_person = 14})
   --   end)

   --initialize the outbox
   --user places both workshop and outbox
   --TODO: make a private stockpile
   --[[
   local outbox_entity = radiant.entities.create_entity('/stonehearth_inventory/entities/stockpile')
   local outbox_location = RadiantIPoint3(-8, 1, -12)
   radiant.terrain.place_entity(outbox_entity, outbox_location)
   local outbox_component = outbox_entity:get_component('radiant:stockpile')
   outbox_component:set_size({3, 3})
   outbox_entity:get_component('unit_info'):set_faction(faction)
   workshop_component:set_outbox(outbox_entity)
   ]]
   -- end TODO

   -- put some items in the world
   --self:place_item_cluster('/stonehearth_trees/entities/oak_tree/oak_log', -10, 10, 3, 3)
   --self:place_item_cluster('/stonehearth_items/cloth_bolt', -7, 10, 2, 2)
end

return PromoteTest

