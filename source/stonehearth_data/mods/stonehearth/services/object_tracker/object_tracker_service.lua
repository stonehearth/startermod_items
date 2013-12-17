
local EntityTracker = require 'services.object_tracker.entity_tracker'
local InventoryTracker = require 'services.object_tracker.inventory_tracker'
local QuantityTracker = require 'services.object_tracker.quantity_tracker'

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
   local tracker_name = 'get_placable_items_tracker:' .. faction

   return self:_find_tracker(tracker_name, function()
      return InventoryTracker()
   end)
end

function ObjectTrackerService:get_resource_tracker(faction)
   local tracker_name = 'get_resource_tracker:' .. faction

   return self:_find_tracker(tracker_name, function()
      --Create filter function for wood and identification
      local filter_fn = function(item)
         assert(item, 'trying to filter a nil item')
         return radiant.entities.is_material(item, 'resource')
      end

      --Returns the means to identify, and its display name, if we know it
      local identifier_fn = function(item)
         --The resource tracker tracks 2 generic categories right now: wood and food
         --TODO: Localize!
         --TODO: Generic icons!
         --TODO: other things can be included generically, but till then, will be
         --tracked by uri
         if radiant.entities.is_material(item, 'wood') then
            return 'wood', 'Wood', '/stonehearth/data/images/resources/log.png'
         elseif radiant.entities.is_material(item, 'food') then
            return 'food', 'Food', '/stonehearth/data/images/resources/food.png'
         else
            return item:get_uri(), nil, nil
         end
      end
      return QuantityTracker(faction, filter_fn, identifier_fn)
   end)
end 

function ObjectTrackerService:get_craftable_tracker(faction)
   local tracker_name = 'get_craftable_tracker:' .. faction

   return self:_find_tracker(tracker_name, function()
      local filter_fn = function(item)
         assert(item, 'trying to filter a nil item')
         return radiant.entities.is_material(item, 'crafted')
      end

      local identifier_fn = function(item)
         return item:get_uri(), nil
      end
      
      return QuantityTracker(faction, filter_fn, identifier_fn)
   end)
end 




return ObjectTrackerService()
