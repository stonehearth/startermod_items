local worker_scheduler = require('stonehearth_worker_class.lib.worker_scheduler')()

local api = {}

function api.promote(entity)
   --local class_api = radiant.mods.require('mod://stonehearth_classes/')
   --class_api.promote(entity, 'mod://stonehearth_worker_class/class_info/')
   --radiant.components.add_component(entity, 'mod://stonehearth_inventory/components/worker_component.lua')
   radiant.entities.inject_into_entity(entity, 'mod://stonehearth_worker_class/class_info/')
end

function api.chop(tree)
   worker_scheduler:schedule_chop(tree)
end

return api
