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
   self._entity = entity
   self._backpack_component = entity:get_component('stonehearth:backpack')

   if not self._backpack_component or self._backpack_component:is_empty() then
      return
   end

   local all_stockpiles = get_all_stockpiles_for_entity(entity)
   self._pathfinder = _radiant.sim.create_astar_path_finder(entity, 'find stockpile for backpack item')

   for id, stockpiles in pairs(all_stockpiles) do
      local stockpile_component = stockpiles:get_component('stonehearth:stockpile')

      if self:_can_currently_stock_backpack_item(stockpile_component) then
         self._pathfinder:add_destination(stockpiles)
      end
   end
   
   self._pathfinder:set_solved_cb(function(path)
         ai:set_think_output({
            path = path,
            stockpile = path:get_destination()
         })
         return true
      end)
   self._pathfinder:start()
end

function FindStockpileForBackpackItem:stop_thinking(ai, entity, args)
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
end

function FindStockpileForBackpackItem:_can_currently_stock_backpack_item(stockpile_component)
   if not stockpile_component or stockpile_component:is_full() then
      return false
   end

   local items = self._backpack_component:get_items()

   for id, item in pairs(items) do
      local filter_fn = stockpile_component:get_filter()
      if filter_fn(item, self._entity) then
         return true
      end
   end

   return false
end

return FindStockpileForBackpackItem
