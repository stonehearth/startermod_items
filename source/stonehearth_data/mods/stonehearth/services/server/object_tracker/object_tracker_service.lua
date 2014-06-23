
local EntityTracker = require 'services.server.object_tracker.entity_tracker'
local InventoryTracker = require 'services.server.object_tracker.inventory_tracker'
local QuantityTracker = require 'services.server.object_tracker.quantity_tracker'

local ObjectTrackerService = class()

function ObjectTrackerService:__init()
   self._all_trackers = {}
end

function ObjectTrackerService:initialize()
   -- object trackers explicitly to not save and restore
   -- state.  it's up to the client to re-attach them after
   -- a load.
end

function ObjectTrackerService:_find_tracker(name, ctor)
   local tracker = self._all_trackers[name]
   if not tracker then
      tracker = ctor()
      self._all_trackers[name] = tracker
   end
   return tracker
end

function ObjectTrackerService:get_worker_tracker(player_id)
   local tracker_name = 'get_worker_tracker:' .. player_id
   local factory_fn = function()
      local filter_fn = function(entity)
         local profession = entity:get_component('stonehearth:profession')

         return profession and 
                profession:get_profession_id() == 'worker' and
                radiant.entities.get_player_id(entity) == player_id
      end
      local event_array = {}
      event_array[1] = {event_name = 'stonehearth:promote'}
      return EntityTracker(tracker_name, filter_fn, event_array)
   end
   return self:_find_tracker(tracker_name, factory_fn)
end

--TODO: return here and use FilteredTrackers for everything
--EVERYTHING!!!
--Right now will finish the caravan scenario and then come back and revist this for 
--score/inventory town view and placeable item UI

--TODO: refactor all trackers to use calls from the inventory service
function ObjectTrackerService:get_placable_items_tracker(player_id)
   local tracker_name = 'get_placable_items_tracker:' .. player_id

   return self:_find_tracker(tracker_name, function()
      return InventoryTracker()
   end)
end

function ObjectTrackerService:get_resource_tracker(player_id)
   local tracker_name = 'get_resource_tracker:' .. player_id

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
         if radiant.entities.is_material(item, 'wood resource') then
            return 'wood', 'Wood', '/stonehearth/data/images/resources/log.png'
         elseif radiant.entities.is_material(item, 'food resource') then
            return 'food', 'Food', '/stonehearth/data/images/resources/food.png'
         else
            return item:get_uri(), nil, nil
         end
      end
      return QuantityTracker(player_id, filter_fn, identifier_fn)
   end)
end 

--[[
--Removing, now use the basic inventory tracker
function ObjectTrackerService:get_craftable_tracker(player_id)
   local tracker_name = 'get_craftable_tracker:' .. player_id

   return self:_find_tracker(tracker_name, function()
      local filter_fn = function(item)
         assert(item, 'trying to filter a nil item')
         return radiant.entities.is_material(item, 'crafted')
      end

      local identifier_fn = function(item)
         return item:get_uri(), nil
      end
      
      return QuantityTracker(player_id, filter_fn, identifier_fn)
   end)
end 
]]
return ObjectTrackerService
