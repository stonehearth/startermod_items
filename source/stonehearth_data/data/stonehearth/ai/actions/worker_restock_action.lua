local WorkerRestockAction = class()

WorkerRestockAction.name = 'stonehearth.restock'
WorkerRestockAction.does = 'stonehearth.restock'
WorkerRestockAction.priority = 5

function WorkerRestockAction:run(ai, entity, path, stockpile)
   local drop_location = path:get_finish_point()
   ai:execute('stonehearth.follow_path', path)
   ai:execute('stonehearth.drop_carrying', drop_location)
   self._reserved = nil
end

return WorkerRestockAction
