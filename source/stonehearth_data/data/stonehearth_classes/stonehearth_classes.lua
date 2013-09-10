local api = {}

function api._promote_outfit(entity, outfit_uri)
   if outfit_uri then
      local outfit = radiant.entities.create_entity(outfit_uri)
      if outfit then
         local render_info = entity:add_component('render_info')
         render_info:attach_entity(outfit)
      end
   end
end

--pass in the dude to promote and the uri of his class_info
function api.promote(entity, uri)
   local class_info = radiant.resources.load_json(uri)
   if class_info then
      api._promote_outfit(entity, class_info.outfit)
   end
end

--radiant.events.register_event('stonehearth.events.classes.demotion')
--radiant.events.register_event('stonehearth.events.classes.promotion')

return api
