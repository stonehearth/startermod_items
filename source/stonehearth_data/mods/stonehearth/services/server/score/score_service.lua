--[[
   Keep track of the scores relevant to different players
]]
local ScoreService = class()
local PlayerScoreData = require 'services.server.score.player_score_data'

function ScoreService:initialize()
   self._sv = self.__saved_variables:get_data()
   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.player_scores = {}
   else
      for player_id, saved_state in pairs(self._sv.player_scores) do 
         local score_data = PlayerScoreData()
         score_data.__saved_variables = saved_state
         score_data:initialize()
         self._sv.player_scores[player_id] = score_data
      end
   end
end

--- Call whenever the aggregate happiness for a player should be updated
function ScoreService:update_town_happiness_score(player_id)
   local pop = stonehearth.population:get_population(player_id)
   local citizens = pop:get_citizens()
   
   if not self._sv.player_scores[player_id] then
      --self._sv.player_scores[player_id] = {}
      self._sv.player_scores[player_id] = PlayerScoreData()
      self._sv.player_scores[player_id].__saved_variables = radiant.create_datastore()
      self._sv.player_scores[player_id]:initialize()
   end

   self:_calculate_town_happiness(player_id, citizens)
end

--- Iterate through all the current citizens and each of their scores. 
--  Every time a score is encountered, update the aggregate local database of
--  scores. When all the citizens and all the scores have been accounted for, 
--  calculate the final scores and score in _sv. If only some people have a score
--  then the score will only be totaled with respect to that number of people.
--  O(n^2) note: yes, this is kind of expensive. However, it is a lot
--  simpler than noting which score has changed and updating only it and all its DEPENDENT
--  scores, given the nested nature of the dependencies. And we expect about 6 scores per person
--  and less than a hundred people. If this does prove to be too expensive, we can revisit.  
function ScoreService:_calculate_town_happiness(player_id, citizens)
   local aggregate_scores = {}
   --Iterate through each citizen
   for id, citizen in pairs(citizens) do
      if citizen then
         local score_component = citizen:get_component('stonehearth:score')
         if score_component then
            --Iterate through each of their scores and calculate the running total for that score
            for name, score_data in pairs(score_component:get_all_scores()) do
               local agg_score_data = aggregate_scores[name]
               if not agg_score_data then
                  agg_score_data = {}
                  agg_score_data.score = score_data.score
                  agg_score_data.num_people = 1
                  aggregate_scores[name] = agg_score_data
               else 
                  agg_score_data.score = agg_score_data.score + score_data.score
                  agg_score_data.num_people = agg_score_data.num_people + 1
               end
            end
         end
      end
   end

   --Calculate final scores for each score available and save them
   local happiness_score = {}
   for name, score_data in pairs(aggregate_scores) do
      happiness_score[name] = score_data.score/score_data.num_people
   end
   --self._sv.player_scores[player_id].happiness = happiness_score
   self._sv.player_scores[player_id]:set_happiness(happiness_score)

   self.__saved_variables:mark_changed()
end


function ScoreService:get_scores_for_player(player_id)
   return self._sv.player_scores[player_id]
end

return ScoreService