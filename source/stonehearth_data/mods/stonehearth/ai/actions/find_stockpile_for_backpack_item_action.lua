local Entity = _radiant.om.Entity
local Path = _radiant.sim.Path
local FindStockpileForBackpackItem = class()

FindStockpileForBackpackItem.name = 'find stockpile for backpack item'
FindStockpileForBackpackItem.does = 'stonehearth:find_stockpile_for_backpack_item'
FindStockpileForBackpackItem.args = {}
FindStockpileForBackpackItem.think_output = {
   stockpile = Entity,
   path = Path,
}
FindStockpileForBackpackItem.version = 2
FindStockpileForBackpackItem.priority = 1

local function get_all_stockpiles_for_entity(entity)
   local player_id = radiant.entities.get_player_id(entity)
   local player_inventory = stonehearth.inventory:get_inventory(player_id)
   return player_inventory:get_all_stockpiles()
end

function FindStockpileForBackpackItem:start_thinking(ai, entity, args)
   self._ai = ai
   self._entity = entity


   self._solved_cb = function(path)
      self._ready = true
      self:_stop_searching()
      ai:set_think_output({
            path = path,
            stockpile = path:get_destination(),
         })
   end

   self._filter_fn = function(candidate)
      local stockpile_component = candidate:get_component('stonehearth:stockpile')
      if not stockpile_component or stockpile_component:is_full() then
         return false
      end
      local items = self._backpack_component:get_items()
      for id, item in pairs(items) do
         local filter_fn = candidate:get_component('stonehearth:storage'):get_filter_function()
         if filter_fn(item) then
            return true
         end
      end
      return false
   end

   self._ready = false
   self._thinking = true
   self:_create_listeners()
   self:_start_searching()
end

function FindStockpileForBackpackItem:stop_thinking(ai, entity, args)
   self._thinking = false
   self:_stop_searching()
   self:_destroy_listeners()
end

function FindStockpileForBackpackItem:_create_listeners()
   if not self._stockpile_item_listener then
      local inventory = stonehearth.inventory:get_inventory(self._entity)
      if inventory then
         self._stockpile_item_listener = radiant.events.listen(inventory, 'stonehearth:inventory:stockpile_added', function()
               self:_start_searching()
            end)
      end
   end
   if not self._backpack_listener then
      self._backpack_listener = radiant.events.listen(self._entity, 'stonehearth:backpack:item_added', self, self._start_searching)
   end
end

function FindStockpileForBackpackItem:_destroy_listeners()
   if self._stockpile_item_listener then
      self._stockpile_item_listener:destroy()
      self._stockpile_item_listener = nil
   end
   if self._backpack_listener then
      self._backpack_listener:destroy()
      self._backpack_listener = nil
   end
end

function FindStockpileForBackpackItem:_start_searching()
   self:_stop_searching()
   if self._ready or not self._thinking then
      return
   end

   local ai = self._ai
   local entity = self._entity

   -- if we don't have a backpack or there's nothing in it, bail.
   self._backpack_component = entity:get_component('stonehearth:backpack')
   if not self._backpack_component or self._backpack_component:is_empty() then
      return
   end

   local pf = entity:add_component('stonehearth:pathfinder')

   pf:clear_filter_fn_cache_entry(self._filter_fn)
   self._pathfinder = pf:find_path_to_entity_type(ai.CURRENT.location,
                             self._filter_fn,
                             'find stockpile for backpack',
                             self._solved_cb)
end

function FindStockpileForBackpackItem:_stop_searching()
   if self._pathfinder then
      self._pathfinder:destroy()
      self._pathfinder = nil
   end
end

return FindStockpileForBackpackItem
