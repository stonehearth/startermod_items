local Inventory = require 'services.server.inventory.inventory'

--[[
   The inventory service tracks, per player, the things in their stockpiles. 
   Given a player_id, you can ask for their inventory.
   You can also create a filtered set of the inventory, via an inventory tracker.

   The default inventory tracker, created by the service for each player,
   sorts the contents of their inventory by uri. 
]]

local InventoryService = class()

function InventoryService:__init()
end

function InventoryService:initialize()   
   self._sv = self.__saved_variables:get_data()
   if self._sv.inventories then
      for faction, ss in pairs(self._sv.inventories) do
         local inventory = Inventory(faction)
         inventory:restore(ss)
         self._sv.inventories[faction] = inventory
      end
   else
      self._sv.inventories = {}
      self._sv.inventory_trackers_by_player = {}
   end

   --Register our functions with the score service
   self:_register_score_functions()
end

function InventoryService:add_inventory(session)
   radiant.check.is_string(session.player_id)
   radiant.check.is_string(session.faction)
   radiant.check.is_string(session.kingdom)

   assert(not self._sv.inventories[session.player_id])

   local inventory = Inventory()
   inventory:initialize(session)
   self._sv.inventories[session.player_id] = inventory
   self:_create_basic_inventory_tracker(session.player_id)

   self.__saved_variables:mark_changed()
   return inventory
end

function InventoryService:get_inventory(player_id)
   radiant.check.is_string(player_id)
   assert(self._sv.inventories[player_id])
   return self._sv.inventories[player_id]
end

--- When an item is added to a player's inventory, tell this function, which will
--  propagate the change to the correct inventory
function InventoryService:add_item(player_id, storage, item)
   local inventory_for_player = self:get_inventory(player_id)
   inventory_for_player:add_item(storage, item)

   --Tell all the traces for this player about this item
   for name, trace in pairs(self._sv.inventory_trackers_by_player[player_id]) do
      trace:add_item(item)
   end
end

--- When an item is removed from a player's inventory, tell this function, which will
--  propgate the change to the correct inventory
function InventoryService:remove_item(player_id, storage, item)
   local inventory_for_player = self:get_inventory(player_id)
   inventory_for_player:remove_item(storage, item)

   --Tell all the traces for this player about this item
   for name, trace in pairs(self._sv.inventory_trackers_by_player[player_id]) do
      trace:remove_item(item)
   end
end

--- Given the uri of an item and the player_id, get a structure containing items of that type
--  Uses the basic_inventory_tracker
--  @param item_type : uri of the item (like stonehearth:oak_log)
--  @param player_id : id of the player
--  @returns an object with a count and a map of identity (items).
--           Iterate through the map to get the data.
--           If entity is nil, that item in the map is now invalid. If the data is nil, there was
--           nothing of that type
function InventoryService:get_items_of_type(item_type, player_id)
   local trackers_for_player = self._sv.inventory_trackers_by_player[player_id]
   if trackers_for_player then
      local basic_inventory_tracker = trackers_for_player['basic_inventory_tracker']
      return basic_inventory_tracker:get_key(item_type)
   end
end

--- Call this function to track a subset of things in this inventory
--  See the documentation for FilteredTracker for the function specifics
function InventoryService:add_inventory_tracker(name, player_id, fn_controller)
   local new_tracker = radiant.create_controller(
      'stonehearth:filtered_tracker',
      name,
      player_id, 
      fn_controller)

   if not self._sv.inventory_trackers_by_player[player_id] then
      self._sv.inventory_trackers_by_player[player_id] = {}
   end
   self._sv.inventory_trackers_by_player[player_id][name] = new_tracker

   self.__saved_variables:mark_changed()

   return new_tracker
end

--- The inventory tracker tracks all objects that go into the inventory (ie, stockpiles)
function InventoryService:_create_basic_inventory_tracker(player_id)
   
   local fn_controller = radiant.create_controller('stonehearth:basic_inventory_tracker')
   self:add_inventory_tracker('basic_inventory_tracker', 
      player_id,
      fn_controller)
end


--Score functions related to inventory (goods you've built, stocked and crafted)
function InventoryService:_register_score_functions()
   --eval function for buildings
   stonehearth.score:add_aggregate_eval_function('net_worth', 'buildings', function(entity, agg_score_bag)
      if entity:get_component('stonehearth:construction_data') then
         agg_score_bag.buildings = agg_score_bag.buildings + self:_get_score_for_building(entity)
      end
   end)

   --eval function for placed items
   stonehearth.score:add_aggregate_eval_function('net_worth', 'placed_item', function(entity, agg_score_bag)
      if entity:get_component('stonehearth:placed_item') then
         local item_value = stonehearth.score:get_score_for_entity_type(entity:get_uri())
         agg_score_bag.placed_item = agg_score_bag.placed_item + item_value
      end
   end)

   --eval function for stockpiles
   stonehearth.score:add_aggregate_eval_function('net_worth', 'stocked_resources', function(entity, agg_score_bag)
      if entity:get_component('stonehearth:stockpile') then
         agg_score_bag.stocked_resources = agg_score_bag.stocked_resources + self:_get_score_for_stockpile(entity)
      end
   end)

   --eval function for food (may replace with using filter for type)
   stonehearth.score:add_aggregate_eval_function('resources', 'edibles', function(entity, agg_score_bag)
      if entity:get_component('stonehearth:stockpile') then
         local stockpile_component = entity:get_component('stonehearth:stockpile')
         local items = stockpile_component:get_items()
         local total_score = 0
         for id, item in pairs(items) do
            if radiant.entities.is_material(item, 'food_container') or radiant.entities.is_material(item, 'food') then
               local item_value = stonehearth.score:get_score_for_entity_type(entity:get_uri())
               agg_score_bag.edibles = agg_score_bag.edibles + item_value
            end
         end
      end
   end)
end

--- The score for a building is its area * the multiplier for that kind of wall/region, divided by 10
function InventoryService:_get_score_for_building(entity)
   local region = entity:get_component('destination'):get_region()
   local area = region:get():get_area()
   local item_multiplier = stonehearth.score:get_score_for_entity_type(entity:get_uri())
   return area * item_multiplier / 10
end

--- Returns the score for all the items in the stockpile. 
--  The score for an item is usually the default (1) but is otherwise defined in score_entity_data.json
function InventoryService:_get_score_for_stockpile(entity)
   local stockpile_component = entity:get_component('stonehearth:stockpile')
   local items = stockpile_component:get_items()
   local total_score = 0
   for id, item in pairs(items) do
      local item_value = stonehearth.score:get_score_for_entity_type(item:get_uri())
      total_score = total_score + item_value
   end
   return total_score
end

return InventoryService

