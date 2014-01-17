local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3

local PromoteTest = class(MicroWorld)
--[[
   Instantiate a worker and a workbench with saw. Promote the worker into a carpenter
]]

function PromoteTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   --Create the carpenter, bench, and instantiate them to each other
   self:place_citizen(0,0)
   local worker = self:place_citizen(12, 12)

   local bench = self:place_item('stonehearth:carpenter:workbench', -12, -12)
   local workshop_component = bench:get_component('stonehearth:workshop')
   local faction = worker:get_component('unit_info'):get_faction()

   --TODO: we need a way to add unitinfo to these all these guys
   bench:add_component('unit_info'):set_faction(faction)

   local outbox_entity = radiant.entities.create_entity('stonehearth:workshop_outbox')
   radiant.terrain.place_entity(outbox_entity, Point3(-9,0,-9))
   outbox_entity:get_component('unit_info'):set_faction(faction)
   local outbox_component = outbox_entity:get_component('stonehearth:stockpile')
   outbox_component:set_size({3, 3})
   outbox_component:set_outbox(true)
   workshop_component:finish_construction(faction, outbox_entity)

   local tree = self:place_tree(-8, 0)
   local tree2 = self:place_tree(-8, 8)
end

return PromoteTest

