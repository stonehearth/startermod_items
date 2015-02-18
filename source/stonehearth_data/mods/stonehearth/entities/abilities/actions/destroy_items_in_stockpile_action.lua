local entity_forms = require 'stonehearth.lib.entity_forms.entity_forms_lib'

local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local DestroyItemsInStockpileAction = class()

DestroyItemsInStockpileAction.name = 'destroy items in stockpile'
DestroyItemsInStockpileAction.does = 'stonehearth:destroy_items_in_stockpile'
DestroyItemsInStockpileAction.args = {
   stockpile = Entity,
}

DestroyItemsInStockpileAction.version = 2
DestroyItemsInStockpileAction.priority = 1


local function get_item_value(item)
   local entity_uri, _, _ = entity_forms.get_uris(item)
   local net_worth = radiant.entities.get_entity_data(entity_uri, 'stonehearth:net_worth')
   if not net_worth then
      return 0
   end
   return net_worth.value_in_gold
end

function DestroyItemsInStockpileAction:start_thinking(ai, entity, args)
   local cheapest_item, cheapest_cost
   local items = args.stockpile:get_component('stonehearth:stockpile')
                                 :get_items()

   -- pick the cheap stuff
   for id, item in pairs(items) do
      if stonehearth.ai:can_acquire_ai_lease(item, entity) then
         local cost = get_item_value(item)
         if not cheapest_cost or cost < cheapest_cost then
            cheapest_item, cheapest_cost = item, cost
            if cost == 0 then
               break
            end
         end
      end
   end
   if cheapest_item then
      ai:set_think_output({ item = cheapest_item })
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(DestroyItemsInStockpileAction)
         :execute('stonehearth:smash_item', { item = ai.PREV.item })
