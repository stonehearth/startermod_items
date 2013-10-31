local WorkerRestockAction = class()

WorkerRestockAction.name = 'restock'
WorkerRestockAction.does = 'stonehearth:restock'
WorkerRestockAction.priority = 5

function WorkerRestockAction:run(ai, entity, path, stockpile)
   local drop_location = path:get_destination_point_of_interest()
   ai:execute('stonehearth:follow_path', path)

   --get some data for analytics
   local carry_block = entity:get_component('carry_block')
   local carried_item_name
   if carry_block then
      local carried_item = carry_block:get_carrying()
      carried_item_name = carried_item:get_component('unit_info'):get_display_name()
   end

   ai:execute('stonehearth:drop_carrying', drop_location)
   self._reserved = nil

   --Log success
   _radiant.analytics.DesignEvent("game:stock_stockpile:worker:" .. carried_item_name)

end

return WorkerRestockAction
