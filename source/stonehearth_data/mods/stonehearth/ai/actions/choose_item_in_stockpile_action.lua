local Entity = _radiant.om.Entity

local ChooseItemInStockpileAction = class()
ChooseItemInStockpileAction.name = 'choose item in stockpile'
ChooseItemInStockpileAction.does = 'stonehearth:choose_item_in_stockpile'
ChooseItemInStockpileAction.args = {
   stockpile = Entity,     -- the stockpile
   filter_fn = {
      type = 'function', -- the function which picks good items
      default = stonehearth.ai.NIL
   }
}
ChooseItemInStockpileAction.think_output = {
   item = Entity           -- the chosen item
}
ChooseItemInStockpileAction.version = 2
ChooseItemInStockpileAction.priority = 1

function ChooseItemInStockpileAction:start_thinking(ai, entity, args)
   local items = args.stockpile:get_component('stonehearth:stockpile')
                                 :get_items()

   local random_item
   local item_count = 1
   local rng = _radiant.csg.get_default_rng()
   local filter_fn = args.filter_fn

   for id, item in pairs(items) do
      if filter_fn == nil or filter_fn(item) then
         if not random_item or rng:get_int(1, item_count) == 1 then
            -- the old, "choose a random item from an infinite stream" trick.
            random_item = item
         end
         item_count = item_count + 1
      end
   end
   if random_item then
      ai:set_think_output({ item = random_item })
   end
end

return ChooseItemInStockpileAction
