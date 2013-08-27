local ch = require 'radiant.core.ch'
local om = require 'radiant.core.om'

ch:register_cmd("radiant.commands.create_stockpile", function(bounds)
   local stockpile = om:create_entity('module://stonehearth/buildings/stockpile')
   
   local origin = Point3(bounds.min)
   bounds.max = bounds.max - bounds.min
   bounds.min = Point3(0, 0, 0)   
   om:place_on_terrain(stockpile, origin)
   
   local designation = om:add_component(stockpile, 'stockpile_designation') 
   designation:set_bounds(bounds)
   designation:set_container(om:get_component(om:get_root_entity(), 'entity_container')) -- xxx do this automatically as part of becoming a child?
   
   om:get_component(stockpile, 'unit_info'):set_faction('civ')
   
   return { entity_id = stockpile:get_id() }
end)
