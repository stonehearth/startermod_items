local om = require 'radiant.core.om'
local ch = require 'radiant.core.ch'
local check = require 'radiant.core.check'

ch:register_cmd("radiant.commands.create_portal", function(wall, kind, location)
   check:is_a(wall, Wall)
   check:is_string(kind)
   check:is_a(location, Point3)
   
   local normal = wall:get_normal()
   local angle = 0
   if normal.x == -1 and normal.z == 0 then
      angle = 90
   elseif normal.x == 0 and normal.z == 1 then
      angle = 180
   elseif normal.x == 1 and normal.z == 0 then
      angle = 270
   end
   
   local entity = om:create_entity(kind)
   local mob = om:add_component(entity, 'mob')
   mob:set_location_grid_aligned(location)
   mob:turn_to(angle)

   local deps = om:add_component(entity, 'build_order_dependencies')
   deps:add_dependency(wall:get_entity())

   local fixture = om:get_component(entity, 'fixture')
   fixture:set_item(om:create_entity(fixture:get_kind()))
   
   wall:add_fixture(entity);
   wall:update_shape()
   
   return { entity_id = entity:get_id() }
end)

