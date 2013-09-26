local WorkerPickupAction = class()

WorkerPickupAction.name = 'stonehearth_worker.actions.pickup_action'
WorkerPickupAction.does = 'stonehearth_worker.actions.pickup'
WorkerPickupAction.priority = 5

function WorkerPickupAction:run(ai, entity, path, item)
   ai:execute('stonehearth.activities.follow_path', path)
   ai:execute('stonehearth.activities.pickup_item', item)
end

return WorkerPickupAction
