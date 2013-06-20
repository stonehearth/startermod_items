local ch = require 'radiant.core.ch'
local om = require 'radiant.core.om'

ch:register_cmd("radiant.commands.create_room", function(bounds)
   local entity = om:create_entity('mod://stonehearth/buildings/room_plan')
   om:add_component(entity, 'unit_info')
   om:set_display_name(entity, 'Room')
   om:set_description(entity, 'Warm and toasty inside')
      
   local room = om:add_component(entity, 'room')
   --room:set_container(om:get_component(sh:get_root_entity(), 'entity_container')) -- xxx do this automatically as part of becoming a child?
   room:set_bounds(bounds)
   om:add_blueprint(entity)
   
   return { entity_id = entity:get_id() }
end)

