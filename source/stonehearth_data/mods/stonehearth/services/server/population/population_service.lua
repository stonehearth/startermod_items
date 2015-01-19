local PopulationFaction = require 'services.server.population.population_faction'
local PopulationService = class()

function PopulationService:initialize()
   self._sv = self.__saved_variables:get_data() 
   if self._sv.populations then
      for player_id, ss in pairs(self._sv.populations) do
         local pop = PopulationFaction(nil, nil, ss) -- xxx: make this a controller!
         self._sv.populations[player_id] = pop
      end
   else
      self._sv.populations = {}
   end
end

function PopulationService:add_population(player_id, kingdom)
   radiant.check.is_string(player_id)
   radiant.check.is_string(kingdom)

   assert(not self._sv.populations[player_id])

   local pop = PopulationFaction(player_id, kingdom, radiant.create_datastore())
   self._sv.populations[player_id] = pop
   return pop
end

function PopulationService:get_population(player_id)
   radiant.check.is_string(player_id)
   assert(self._sv.populations[player_id])
   return self._sv.populations[player_id]
end

function PopulationService:get_population_size(player_id)
   radiant.check.is_string(player_id)
   assert(self._sv.populations[player_id])
   local pop_faction = self._sv.populations[player_id]
   local citizens = pop_faction:get_citizens()
   local num_citizens = 0
   for id, citizen in pairs(citizens) do
      if citizen then 
         num_citizens = num_citizens + 1
      end
   end 
   return num_citizens
end

function PopulationService:get_all_populations()
   return self._sv.populations
end

-- return all populations which are friendly to the specified player
function PopulationService:get_friendly_populations(player_id)
   local result = {}
   for other_player_id, pop in pairs(self._sv.populations) do
      -- kind of a lame definition of "friendly", but we can fix when we get
      -- to multiplayer.
      if other_player_id == player_id then
         result[other_player_id] = pop
      end
   end
   return result
end

function PopulationService:get_population_command(session, response)
   return { uri = self:get_population(session.player_id) }
end

return PopulationService
