local PopulationFaction = require 'services.population.population_faction'
local PopulationService = class()

function PopulationService:__init()
   self._factions = {}
end

function PopulationService:initialize()   
   self.__savestate = radiant.create_datastore({
         factions = self._factions
      })
end

function PopulationService:restore(savestate)
   self.__savestate = savestate
   self.__savestate:read_data(function(o)
         for faction, ss in pairs(o.factions) do
            local pop = PopulationFaction(faction)
            pop:restore(ss)
            self._factions[faction] = pop
         end
      end)
end


--TODO: civ is assumed to be the user faction in many places. Stamp this out!
function PopulationService:get_faction(faction, kingdom)
   radiant.check.is_string(faction)
   local pop = self._factions[faction]
   if not pop and kingdom then
      pop = PopulationFaction()
      pop:initialize(faction, kingdom)
      self._factions[faction] = pop
   end
   return pop
end

function PopulationService:get_all_factions()
   return self._factions
end

return PopulationService
