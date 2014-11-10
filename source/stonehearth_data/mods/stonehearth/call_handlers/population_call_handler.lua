local Population = stonehearth.Population

local PopulationCallHandler = class()
local all_traces = {}

function PopulationCallHandler:get_population(session, response)
   return { population = stonehearth.population:get_population(session.player_id) }
end

function PopulationCallHandler:get_tasks(session, response)
   local town = stonehearth.town:get_town(session.player_id)
   if not town then
      response:reject('could not find town')
   end
   local tasks_model = town:get_scheduler()
                              :get_model()

   return { tasks = tasks_model }
end

return PopulationCallHandler
