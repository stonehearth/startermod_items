local WorkerRestockAction = class()

WorkerRestockAction.name = 'stonehearth_worker.actions.restock_action'
WorkerRestockAction.does = 'stonehearth_worker.actions.restock'
WorkerRestockAction.priority = 5

function WorkerRestockAction:run(ai, entity, path, stockpile, drop_location)
   ai:execute('stonehearth.activities.follow_path', path)
   ai:execute('stonehearth.activities.drop_carrying', drop_location)
   self._reserved = nil
end

return WorkerRestockAction
