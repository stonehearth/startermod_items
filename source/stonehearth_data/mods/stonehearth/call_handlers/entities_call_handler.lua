
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

return EntityCallHandler
