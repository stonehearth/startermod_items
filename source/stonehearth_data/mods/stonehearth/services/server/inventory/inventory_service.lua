local Entity = _radiant.om.Entity

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
   if not self._sv.inventories then
      self._sv.inventories = {}
   end

   --Register our functions with the score service
   self:_register_score_functions()
end

function InventoryService:add_inventory(player_id)
   radiant.check.is_string(player_id)

   assert(not self._sv.inventories[player_id])

   local inventory = radiant.create_controller('stonehearth:inventory', player_id)
   assert(inventory)
   
   self._sv.inventories[player_id] = inventory
   self.__saved_variables:mark_changed()
   return inventory
end

function InventoryService:get_inventory(arg1)
   local player_id
   if radiant.util.is_a(arg1, 'string') then
      player_id = arg1
   elseif radiant.util.is_a(arg1, Entity) then
      player_id = radiant.entities.get_player_id(arg1)
   else
      error(string.format('unexpected value %s in get_inventory', radiant.util.tostring(player_id)))
   end
   radiant.check.is_string(player_id)
   
   if self._sv.inventories[player_id] then
      return self._sv.inventories[player_id]
   end
end

function InventoryService:get_item_tracker_command(session, response, tracker_name)
   local inventory = self:get_inventory(session.player_id)

   if inventory == nil then
      response:reject('there is no inventory for player ' .. session.player_id)
      return
   end

   return { tracker = inventory:get_item_tracker(tracker_name) }
end

--Score functions related to inventory (goods you've built, stocked and crafted)
--
-- TODO: move these registration functions to the site where the item is implemented
-- rather than sticking them all in inventory.  for example, accumuating buildings into
-- net worth should be registered from stonehearth.build -- tony
--
function InventoryService:_register_score_functions()
   --eval function for buildings
   stonehearth.score:add_aggregate_eval_function('net_worth', 'buildings', function(entity, agg_score_bag)
      if entity:get_component('stonehearth:construction_data') then
         agg_score_bag.buildings = agg_score_bag.buildings + self:_get_score_for_building(entity)
      end
   end)

   --eval function for placed items
   stonehearth.score:add_aggregate_eval_function('net_worth', 'placed_item', function(entity, agg_score_bag)
      if entity:get_component('stonehearth:entity_forms') then
         local item_value = stonehearth.score:get_score_for_entity(entity)
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
               local item_value = stonehearth.score:get_score_for_entity(item)
               agg_score_bag.edibles = agg_score_bag.edibles + item_value
            end
         end
      end
   end)
end

--- The score for a building is its area * the multiplier for that kind of wall/region, modified by ^0.6, 
--  so that the first bldg is very important, and building decreases logarithmically as we get more buildings.
function InventoryService:_get_score_for_building(entity)
   local region = entity:get_component('destination'):get_region()
   local area = region:get():get_area()
   local item_multiplier = stonehearth.score:get_score_for_entity(entity)
   return (area * item_multiplier) ^ 0.5
end

--- Returns the score for all the items in the stockpile. 
--  The score for an item is usually the default (1) but is otherwise defined in entity's entity_data
function InventoryService:_get_score_for_stockpile(entity)
   local stockpile_component = entity:get_component('stonehearth:stockpile')
   local items = stockpile_component:get_items()
   local total_score = 0
   for id, item in pairs(items) do
      local item_value = stonehearth.score:get_score_for_entity(item)
      total_score = total_score + item_value
   end
   return total_score / 10
end

return InventoryService
