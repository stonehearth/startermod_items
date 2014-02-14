local PopulationFaction = require 'services.population.population_faction'
local PopulationService = class()

function PopulationService:__init()
   self._factions = {}
end

--TODO: civ is assumed to be the user faction in many places. Stamp this out!
function PopulationService:get_faction(faction, kingdom)
   radiant.check.is_string(faction)
   local scheduler = self._factions[faction]
   if not scheduler and kingdom then
      scheduler = PopulationFaction(faction, kingdom)
      self._factions[faction] = scheduler
   end
   return scheduler
end

function PopulationService:get_all_factions()
   return self._factions
end

return PopulationService()
