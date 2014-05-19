local PopulationFaction = require 'services.server.population.population_faction'
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
   self:_register_score_functions()
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

--Keep track of the awesomeness of the job levels of the town (for use with scenario service?)
function PopulationService:_register_score_functions()
   --If the entity is a farm, register the score
   stonehearth.score:add_aggregate_eval_function('people_power', 'citizens', function(entity, agg_score_bag)
      if entity:get_component('stonehearth:profession') then
         agg_score_bag.citizens = agg_score_bag.citizens + self:_get_score_for_citizen(entity)
      end
   end)
end

function PopulationService:_get_score_for_citizen(entity)
   local alias = entity:get_component('stonehearth:profession'):get_profession_uri()

   --If such a data exists, then this entity is a person with a job. Count them!
   local data = radiant.resources.load_json(alias)
   --TODO: add score_value to the data
   if data and data.score_value then
      --TODO: add or multiply?
      return stonehearth.constants.score.DEFAULT_CIV_WORTH + data.score_value
   else
      return stonehearth.constants.score.DEFAULT_CIV_WORTH
   end
end


return PopulationService
