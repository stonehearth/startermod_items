local WorkerDefenseCallHandler = class()

function WorkerDefenseCallHandler:enable_worker_combat(session, response)
   stonehearth.worker_defense:enable_worker_combat(session.player_id)
   response:resolve({})
end

function WorkerDefenseCallHandler:disable_worker_combat(session, response)
   stonehearth.worker_defense:disable_worker_combat(session.player_id)
   response:resolve({})
end

return WorkerDefenseCallHandler
