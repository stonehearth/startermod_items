
local EntityTracker = require 'lib.object_tracker.entity_tracker'
local InventoryTracker = require 'lib.object_tracker.inventory_tracker'
local TrackerManager = class()

function TrackerManager:__init()
   self._all_trackers = {}
end

function TrackerManager:_find_tracker(name, ctor)
   local tracker = self._all_trackers[name]
   if not tracker then
      tracker = ctor()
      self._all_trackers[name] = tracker
   end
   return tracker
end

function TrackerManager:get_worker_tracker(faction)
   local tracker_name = 'get_worker_tracker:' .. faction
   local factory_fn = function()
      local filter_fn = function(entity)
         local job_info = entity:get_component('stonehearth:job_info')

         return job_info and 
                job_info:get_id() == 'worker' and
                radiant.entities.get_faction(entity) == faction
      end
      return EntityTracker(filter_fn)
   end
   return self:_find_tracker(tracker_name, factory_fn)
end

function TrackerManager:get_placable_items_tracker(faction)
   local tracker_name = 'get_worker_tracker:' .. faction

   return self:_find_tracker(tracker_name, function()
      return InventoryTracker()
   end)
end

return TrackerManager()
