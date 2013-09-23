
local EntityTracker = require 'entity_tracker'
local InventoryTracker = radiant.mods.require('stonehearth_inventory.inventory_tracker')

-- a new TrackerManager gets created to handle every request for a new tracker.
-- Keep the global list of trackers outside that class, so we can persist them
-- across calls.
local all_trackers = {}
local find_tracker = function(name, ctor)
   local tracker = all_trackers[name]
   if not tracker then
      tracker = ctor()
      all_trackers[name] = tracker
   end
   return {
      tracker = tracker:get_data_store()
   }
end

local TrackerManager = class()

-- xxx: merge this with the inventory tracker manager!
function TrackerManager:get_worker_tracker(session, response)
   local faction = session.faction
   local tracker_name = 'get_worker_tracker:' .. faction
   local factory_fn = function()
      local filter_fn = function(entity)
         local job_info = entity:get_component('stonehearth_classes:job_info')

         return job_info and 
                job_info:get_id() == 'worker' and
                radiant.entities.get_faction(entity) == faction
      end
      return EntityTracker(filter_fn)
   end
   return find_tracker(tracker_name, factory_fn)
end

function TrackerManager:get_placable_items_tracker(session, response)
   local tracker_name = 'get_worker_tracker:' .. session.faction

   return find_tracker(tracker_name, function()
      return InventoryTracker()
   end)
end


return TrackerManager
