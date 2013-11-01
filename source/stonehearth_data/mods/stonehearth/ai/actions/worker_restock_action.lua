local WorkerRestockAction = class()

WorkerRestockAction.name = 'restock'
WorkerRestockAction.does = 'stonehearth:restock'
WorkerRestockAction.priority = 5

function WorkerRestockAction:run(ai, entity, path, stockpile)
   local drop_location = path:get_destination_point_of_interest()
   ai:execute('stonehearth:follow_path', path)
   ai:execute('stonehearth:drop_carrying', drop_location)
   self._reserved = nil
end

return WorkerRestockAction
