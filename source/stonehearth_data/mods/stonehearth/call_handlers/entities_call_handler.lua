
local EntityCallHandler = class()

function EntityCallHandler:set_display_name(session, response, entity, name)
   assert(entity)

   entity:add_component('unit_info'):set_display_name(name)
   return true
end

function EntityCallHandler:destroy_entity(session, response, entity)
   assert(entity)
   
   radiant.entities.destroy_entity(entity)
   return true
end

function EntityCallHandler:kill_entity(session, response, entity)
   assert(entity)
   
   radiant.entities.kill_entity(entity)
   return true
end

function EntityCallHandler:reset_entity(session, response, entity)
   assert(entity)
   local location = radiant.entities.get_world_grid_location(entity)
   local placement_point = radiant.terrain.find_placement_point(location, 1, 4)
   radiant.terrain.place_entity(entity, placement_point)
   return true
end

return EntityCallHandler
