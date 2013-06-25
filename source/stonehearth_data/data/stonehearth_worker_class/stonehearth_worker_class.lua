local api = {}

function api.promote(entity)
   --local class_api = radiant.mods.require('/stonehearth_classes/')
   --class_api.promote(entity, '/stonehearth_worker_class/class_info/')
   --radiant.components.add_component(entity, '/stonehearth_inventory/components/worker_component.lua')
   radiant.entities.inject_into_entity(entity, '/stonehearth_worker_class/class_info/')
end

return api
