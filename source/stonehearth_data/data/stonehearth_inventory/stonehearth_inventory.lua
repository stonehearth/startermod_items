local api = {}

function api.create_stockpile(bounds)
   local stockpile = radiant.entities.create_entity('mod://stonehearth_inventory/entities/stockpile')
   
   local origin = RadiantIPoint3(bounds.min)
   bounds.max = bounds.max - bounds.min
   bounds.min = RadiantIPoint3(0, 0, 0)   
   radiant.terrain.place_entity(stockpile, origin)
   
   local designation = radiant.components.add_component(stockpile, 'stockpile_designation') 
   designation:set_bounds(bounds)

   --designation:set_container(om:get_component(om:get_root_entity(), 'entity_container')) -- xxx do this automatically as part of becoming a child? 
   
   local unitinfo = radiant.components.add_component(stockpile, 'unit_info')
   unitinfo:set_faction('civ')
   
   return { entity_id = stockpile:get_id() }
end

return api