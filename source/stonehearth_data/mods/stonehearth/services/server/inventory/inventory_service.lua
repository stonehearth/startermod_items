local Inventory = require 'services.server.inventory.inventory'

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
   self.__saved_variables:mark_changed()
   return inventory
end

function InventoryService:get_inventory(player_id)
   radiant.check.is_string(player_id)
   assert(self._sv.inventories[player_id])
   return self._sv.inventories[player_id]
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
         -- TODO: add higher scores to some things, otherwise they all count as 1
         agg_score_bag.stocked_resources = agg_score_bag.stocked_resources + self:_get_score_for_stockpile(entity)
      end
   end)
end

--- The score for a building is its area * the multiplier for that kind of wall/region
function InventoryService:_get_score_for_building(entity)
   local region = entity:get_component('destination'):get_region()
   local area = region:get():get_area()
   local item_multiplier = stonehearth.score:get_score_for_entity_type(entity:get_uri())
   return area * item_multiplier
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

