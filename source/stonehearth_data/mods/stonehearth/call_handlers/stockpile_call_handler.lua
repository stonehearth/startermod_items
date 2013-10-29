local StockpileCallHandler = class()

function StockpileCallHandler:set_stockpile_filter(session, response, stockpile_entity, values)
   local stockpile_component = stockpile_entity:add_component('stonehearth:stockpile')

   stockpile_component:set_filter(values)
   return true
end

return StockpileCallHandler
