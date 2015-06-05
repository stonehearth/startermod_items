local BackpackComponent = class()

local log = radiant.log.create_logger('backpack')

function BackpackComponent:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()
   self._entity = entity

   self._filter = entity:add_component('stonehearth:storage_filter')
   if not self._sv.initialized then
      self._sv.items = {}
      self._sv.num_items = 0
      self._sv.capacity = json.capacity or 8
      self._sv.initialized = true
   end

   self._kill_listener = radiant.events.listen(entity, 'stonehearth:kill_event', self, self._on_kill_event)
end

function BackpackComponent:get_filter()
   return self._filter:get_filter_function()
end

function BackpackComponent:destroy()
   self._kill_listener:destroy()
   self._kill_listener = nil
end

-- Call to increase/decrease backpack size
-- @param capacity_change - add this number to capacity. For decrease, us a negative number
function BackpackComponent:change_max_capacity(capacity_change)
   self._sv.capacity = self._sv.capacity + capacity_change
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

   local id = item:get_id()

   if not self._sv.items[id] then
      local player_id = radiant.entities.get_player_id_from_entity(self._entity)
      stonehearth.inventory:get_inventory(player_id):update_item_container(id, self._entity)
      self._sv.items[id] = item
      self._sv.num_items = self._sv.num_items + 1
      self.__saved_variables:mark_changed()
      radiant.events.trigger_async(self._entity, 'stonehearth:backpack:item_added')
   end

   return true
end

function BackpackComponent:remove_item(item)
   local id = item:get_id()

   if self._sv.items[id] then
      local player_id = radiant.entities.get_player_id_from_entity(self._entity)
      stonehearth.inventory:get_inventory(player_id):update_item_container(id, nil)
      self._sv.items[id] = nil
      self._sv.num_items = self._sv.num_items - 1
      self.__saved_variables:mark_changed()
      radiant.events.trigger_async(self._entity, 'stonehearth:backpack:item_removed')
      return true
   end

   return false
end

function BackpackComponent:contains_item(item)
   for id, backpack_item in pairs(self._sv.items) do
      if backpack_item == item then
         return true
      end
   end
   
   return false
end

function BackpackComponent:get_items()
   return self._sv.items
end

function BackpackComponent:num_items()
   return self._sv.num_items
end

function BackpackComponent:capacity()
   return self._sv.capacity
end

function BackpackComponent:is_empty()
   return self._sv.num_items == 0
end

function BackpackComponent:is_full()
   return self._sv.num_items >= self._sv.capacity
end

function BackpackComponent:remove_first_item()
   local id, item = next(self._sv.items)

   if item then
      self:remove_item(item)
   end
   
   return item
end

return BackpackComponent
