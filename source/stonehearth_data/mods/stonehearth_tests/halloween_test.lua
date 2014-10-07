local MicroWorld = require 'lib.micro_world'
local HalloweenTest = class(MicroWorld)
local Point3 = _radiant.csg.Point3

function HalloweenTest:__init()
   self[MicroWorld]:__init(64)
   self:create_world()

   self:place_item('stonehearth:large_oak_tree', -25, -25)
   self:place_citizen(20, 20)

   --[[
   local player_id = worker:get_component('unit_info'):get_player_id()
   local town = stonehearth.town:get_town(player_id)
   local location = Point3(-7, 0, -7)
   local banner_entity = radiant.entities.create_entity('stonehearth:camp_standard')
   radiant.terrain.place_entity(banner_entity, location)
   town:set_banner(banner_entity)

   self:place_item_cluster('stonehearth:oak_log', -10, 0, 10, 10)
   --self:place_item_cluster('stonehearth:cloth_bolt', 10, 3, 3, 3)
   ]]

   -- Introduce a new person/scenario 
   self:at(500,  function()
         stonehearth.dynamic_scenario:force_spawn_scenario('stonehearth:scenarios:skeleton_invasion')
      end)
end

return HalloweenTest