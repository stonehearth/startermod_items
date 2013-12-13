local StockpileCallHandler = class()

function StockpileCallHandler:set_stockpile_filter(session, response, stockpile_entity, filter)
   local stockpile_component = stockpile_entity:add_component('stonehearth:stockpile')

   stockpile_component:set_filter(filter)
   return true
end

return StockpileCallHandler
