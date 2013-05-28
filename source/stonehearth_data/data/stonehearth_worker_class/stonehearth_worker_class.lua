local class_info = radiant.resources.load_json('mod://stonehearth_worker_class/class/class_info.txt')
local worker_scheduler = require('stonehearth_worker_class.lib.worker_scheduler')

local api = {}

function api.promote(entity)
   if class_info then
      for _, outfit in radiant.resources.pairs(class_info.outfits) do
         radiant.entities.add_outfit(entity, outfit)
      end
   end
end

function api.chop(tree)
   worker_scheduler.schedule_chop(tree)
end

return api
