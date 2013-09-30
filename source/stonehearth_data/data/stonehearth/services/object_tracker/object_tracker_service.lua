
local EntityTracker = require 'services.object_tracker.entity_tracker'
local InventoryTracker = require 'services.object_tracker.inventory_tracker'
local ObjectTrackerService = class()

function ObjectTrackerService:__init()
   self._all_trackers = {}
end

function ObjectTrackerService:_find_tracker(name, ctor)
   local tracker = self._all_trackers[name]
   if not tracker then
      tracker = ctor()
      self._all_trackers[name] = tracker
   end
   return tracker
end

function ObjectTrackerService:get_worker_tracker(faction)
   local tracker_name = 'get_worker_tracker:' .. faction
   local factory_fn = function()
      local filter_fn = function(entity)
         local profession = entity:get_component('stonehearth:profession')

         return profession and 
                profession:get_profession_type() == 'worker' and
                radiant.entities.get_faction(entity) == faction
      end
      return EntityTracker(filter_fn)
   end
   return self:_find_tracker(tracker_name, factory_fn)
end

function ObjectTrackerService:get_placable_items_tracker(faction)
   local tracker_name = 'get_worker_tracker:' .. faction

   return self:_find_tracker(tracker_name, function()
      return InventoryTracker()
   end)
end

return ObjectTrackerService()
