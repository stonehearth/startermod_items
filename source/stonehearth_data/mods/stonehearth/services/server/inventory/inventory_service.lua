local Inventory = require 'services.server.inventory.inventory'

local InventoryService = class()

function InventoryService:__init()
   self._inventories = {}
end

function InventoryService:initialize()   
   local data = self.__saved_variables:get_data()
   if data.inventories then
      for faction, ss in pairs(data.inventories) do
         local inventory = Inventory(faction)
         inventory:restore(ss)
         self._inventories[faction] = inventory
      end      
   else
      self._inventories = {}
      data.inventories = self._inventories
   end

   --Register our functions with the score service
   self:_register_score_functions()
end

function InventoryService:restore(saved_variables)
   self.__saved_variables = saved_variables
   self.__saved_variables:read_data(function(o)
         for faction, ss in pairs(o.inventories) do
            local inventory = Inventory(faction)
            inventory:restore(ss)
            self._inventories[faction] = inventory
         end
      end)
end

function InventoryService:add_inventory(session)
   radiant.check.is_string(session.player_id)
   radiant.check.is_string(session.faction)
   radiant.check.is_string(session.kingdom)

   assert(not self._inventories[session.player_id])

   local inventory = Inventory()
   inventory:initialize(session)
   self._inventories[session.player_id] = inventory
   self.__saved_variables:mark_changed()
   return inventory
end

function InventoryService:get_inventory(player_id)
   radiant.check.is_string(player_id)
   assert(self._inventories[player_id])
   return self._inventories[player_id]
end

--Score functions related to inventory (goods you've built, stocked and crafted)
function InventoryService:_register_score_functions()
   --eval function for buildings
   stonehearth.score:add_net_worth_eval_function('buildings', function(entity, net_worth_score)
      if entity:get_component('stonehearth:construction_data') then
         net_worth_score.buildings = net_worth_score.buildings + self:_get_score_for_building(entity)
      end
   end)

   --eval function for placed items
   stonehearth.score:add_net_worth_eval_function('placed_item', function(entity, net_worth_score)
      if entity:get_component('stonehearth:placed_item') then
         -- TODO: add higher scores to some things, otherwise they all count as 1
         net_worth_score.placed_item = net_worth_score.placed_item + 1
      end
   end)

   --eval function for stockpiles
   stonehearth.score:add_net_worth_eval_function('stocked_resources', function(entity, net_worth_score)
      if entity:get_component('stonehearth:stockpile') then
         -- TODO: add higher scores to some things, otherwise they all count as 1
         net_worth_score.stocked_resources = net_worth_score.stocked_resources + self:_get_score_for_stockpile(entity)
      end
   end)
end

function InventoryService:_get_score_for_building(entity)
   local region = entity:get_component('destination'):get_region()
   local area = region:get():get_area()
   --TODO: BASED ON THE TYPE OF THE ENTITY, CHANGE THE WORTH OF THE REGION by multiplying it by a score value
   return area
end

function InventoryService:_get_score_for_stockpile(entity)
   local stockpile_component = entity:get_component('stonehearth:stockpile')
   local items = stockpile_component:get_items()
   local total_score = 0
   for id, item in pairs(items) do
      --TODO: annotate some items with different scores
      total_score = total_score + 1
   end
   return total_score
end

return InventoryService

