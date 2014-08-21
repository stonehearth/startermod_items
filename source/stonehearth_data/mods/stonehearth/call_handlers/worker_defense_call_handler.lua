local WorkerDefenseCallHandler = class()

function WorkerDefenseCallHandler:enable_worker_combat(session, response)
   local town = stonehearth.town:get_town(session.player_id)
   town:enable_worker_combat()
   response:resolve({})
end

function WorkerDefenseCallHandler:disable_worker_combat(session, response)
   local town = stonehearth.town:get_town(session.player_id)
   town:disable_worker_combat()
   response:resolve({})
end

function WorkerDefenseCallHandler:worker_combat_enabled(session, response)
   local town = stonehearth.town:get_town(session.player_id)
   local enabled = town:worker_combat_enabled()
   return { enabled = enabled }
end

return WorkerDefenseCallHandler
