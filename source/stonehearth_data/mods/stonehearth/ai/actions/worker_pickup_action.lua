local WorkerPickupAction = class()

WorkerPickupAction.name = 'pickup action'
WorkerPickupAction.does = 'stonehearth_worker:actions:pickup'
WorkerPickupAction.priority = 5

function WorkerPickupAction:run(ai, entity, path, item)
   ai:execute('stonehearth:follow_path', path)
   ai:execute('stonehearth:pickup_item', item)
end

return WorkerPickupAction
