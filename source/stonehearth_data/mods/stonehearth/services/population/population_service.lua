local PopulationFaction = require 'services.population.population_faction'
local PopulationService = class()

function PopulationService:initialize()
   self._sv = self.__saved_variables:get_data() 
   if self._sv.populations then
      for player_id, ss in pairs(self._sv.populations) do
         local pop = PopulationFaction(nil, ss)
         self._sv.populations[player_id] = pop
      end
   else
      self._sv.populations = {}
   end
end


function PopulationService:add_population(session)
   radiant.check.is_string(session.player_id)
   radiant.check.is_string(session.faction)
   radiant.check.is_string(session.kingdom)

   assert(not self._sv.populations[session.player_id])

   local pop = PopulationFaction(session, radiant.create_datastore())
   self._sv.populations[session.player_id] = pop
   return pop
end

function PopulationService:get_population(player_id)
   radiant.check.is_string(player_id)
   assert(self._sv.populations[player_id])
   return self._sv.populations[player_id]
end

function PopulationService:get_all_populations()
   return self._sv.populations
end

-- return all populations which are friendly to the specified
-- faction
function PopulationService:get_friendly_populations(faction)
   local result = {}
   for player_id, pop in pairs(self._sv.populations) do
      -- kind of a lame definition of "friendly", but we can fix when we get
      -- to multiplayer.
      if pop:get_faction() == faction then
         result[player_id] = pop
      end
   end
   return result
end

return PopulationService
