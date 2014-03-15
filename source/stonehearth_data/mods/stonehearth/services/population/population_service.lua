local PopulationFaction = require 'services.population.population_faction'
local PopulationService = class()

function PopulationService:__init()
   self._populations = {}
end

function PopulationService:initialize()   
   self.__saved_variables = radiant.create_datastore({
         factions = self._populations
      })
end

function PopulationService:restore(saved_variables)
   self.__saved_variables = saved_variables
   self.__saved_variables:read_data(function(o)
         for faction, ss in pairs(o.factions) do
            local pop = PopulationFaction(faction)
            pop:restore(ss)
            self._populations[faction] = pop
         end
      end)
end


function PopulationService:add_population(session)
   radiant.check.is_string(session.player_id)
   radiant.check.is_string(session.faction)
   radiant.check.is_string(session.kingdom)

   assert(not self._populations[session.player_id])

   local pop = PopulationFaction()
   pop:initialize(session)
   self._populations[session.player_id] = pop
   return pop
end

--TODO: civ is assumed to be the user faction in many places. Stamp this out!
function PopulationService:get_population(player_id)
   radiant.check.is_string(player_id)
   assert(self._populations[player_id])
   return self._populations[player_id]
end

function PopulationService:get_all_populations()
   return self._populations
end

-- return all populations which are friendly to the specified
-- faction
function PopulationService:get_friendly_populations(faction)
   local result = {}
   for player_id, pop in pairs(self._populations) do
      -- kind of a lame definition of "friendly", but we can fix when we get
      -- to multiplayer.
      if pop:get_faction() == faction then
         result[player_id] = pop
      end
   end
   return result
end

return PopulationService
