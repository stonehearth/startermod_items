local Population = stonehearth.Population

local PopulationCallHandler = class()
local all_traces = {}

function PopulationCallHandler:get_population(session, response)
   return { population = stonehearth.population:get_population(session.player_id) }
end

return PopulationCallHandler
