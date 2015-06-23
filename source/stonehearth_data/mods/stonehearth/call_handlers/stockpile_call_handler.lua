local StockpileCallHandler = class()

function StockpileCallHandler:set_stockpile_filter(session, response, storage_entity, filter)
   if radiant.check.is_entity(storage_entity) then
      local storage_component = storage_entity:get_component('stonehearth:storage')
      assert(storage_component)
      storage_component:set_filter(filter)
   end
   return true
end

return StockpileCallHandler
