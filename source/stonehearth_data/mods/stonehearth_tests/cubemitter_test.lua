local MicroWorld = require 'lib.micro_world'
local HarvestTest = class(MicroWorld)

function HarvestTest:__init()
   self[MicroWorld]:__init(64)
   self:create_world()

   self:place_item('stonehearth:large_oak_tree', -25, -25)
   self:place_item('stonehearth:medium_oak_tree', -5, -25)
   self:place_item('stonehearth:small_oak_tree',  15, -25)

   self:place_item('stonehearth:large_juniper_tree', -25, -5)
   self:place_item('stonehearth:medium_juniper_tree', -5, -5)
   self:place_item('stonehearth:small_juniper_tree',  15, -5)

   local effect_entity = self:place_item('stonehearth:berry_bush', -0, 0)
   local effect = '/stonehearth/data/effects/object_destroyed/object_destroyed.json'

   radiant.effects.run_effect(effect_entity, effect)
end

return HarvestTest

