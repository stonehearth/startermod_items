local BackpackComponent = class()

local constants = require 'constants'
local log = radiant.log.create_logger('backpack')

function BackpackComponent:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()
   self._entity = entity
   self.num_reserved = 0

   if not self._sv.initialized then
      self._sv.capacity = json.capacity or 8
      self._sv.initialized = true
   end

   self._kill_listener = radiant.events.listen(entity, 'stonehearth:kill_event', self, self._on_kill_event)
   self._filter_listener = radiant.events.listen(entity, 'stonehearth:storage:filter_changed', self, self._on_filter_changed)
end

function BackpackComponent:reserve_space()
   if self:_get_storage():get_num_items() + self.num_reserved >= self._sv.capacity then
      return false
   end
   self.num_reserved = self.num_reserved + 1
   return true
end

function BackpackComponent:_get_storage()
   return self._entity:get_component('stonehearth:storage')
end

function BackpackComponent:unreserve_space()
   if self.num_reserved > 0 then
      self.num_reserved = self.num_reserved - 1
   end
end

function BackpackComponent:destroy()
   self._kill_listener:destroy()
   self._kill_listener = nil

   self._filter_listener:destroy()
   self._filter_listener = nil
end

-- Call to increase/decrease backpack size
-- @param capacity_change - add this number to capacity. For decrease, us a negative number
function BackpackComponent:change_max_capacity(capacity_change)
   self._sv.capacity = self._sv.capacity + capacity_change
end

function BackpackComponent:_on_filter_changed(filter_component, newly_filtered, newly_passed)
   -- If this backpack is on an entity that has a filter, and that filter changes, let the ai
   -- know about entities that no longer pass the filter, so that they can take another swipe at
   -- doing something with them.
   for id, item in pairs(newly_filtered) do
      radiant.events.trigger_async(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', item)
   end

   -- Let the AI know that _we_ (the backpack) have changed, so reconsider us, too!
   radiant.events.trigger_async(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', self._entity)
end

--If we're killed, dump the things in our backpack
function BackpackComponent:_on_kill_event()
   -- npc's don't drop what's in their pack
   if not stonehearth.player:is_npc(self._entity) then
      while not self:is_empty() do
         local item = self:remove_first_item()
         local location = radiant.entities.get_world_grid_location(self._entity)
         local placement_point = radiant.terrain.find_placement_point(location, 1, 4)
         radiant.terrain.place_entity(item, placement_point)
      end
   end
end
   
function BackpackComponent:add_item(item)
   if self:is_full() then
      return false
   end

   if self:contains_item(item:get_id()) then
      return true
   end

   self:_get_storage():add_item(item)
   radiant.events.trigger_async(self._entity, 'stonehearth:backpack:item_added')

   return true
end

function BackpackComponent:remove_item(item)
   local id = item:get_id()

   if not self:contains_item(id) then
      return false
   end

   self:_get_storage():remove_item(id)
   radiant.events.trigger_async(self._entity, 'stonehearth:backpack:item_removed')

   return true
end

function BackpackComponent:contains_item(id)
   return self:_get_storage():item_with_id(id) ~= nil
end

function BackpackComponent:get_items()
   return self:_get_storage():get_items()
end

function BackpackComponent:num_items()
   return self:_get_storage():get_num_items()
end

function BackpackComponent:capacity()
   return self._sv.capacity
end

function BackpackComponent:is_empty()
   return self:num_items() == 0
end

function BackpackComponent:is_full()
   return self:num_items() >= self._sv.capacity
end

function BackpackComponent:remove_first_item()
   local id, item = next(self:get_items())

   if item then
      self:remove_item(item)
   end
   
   return item
end

return BackpackComponent
