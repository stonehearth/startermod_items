local log = radiant.log.create_logger('score')

--[[
   Keep track of the scores relevant to different players

   -  Cumulative scores for the whole town. (For example, net worth for the inventory for the whole town, etc.)
      Every 10 minutes of in-game time, we calculate the cumulative scores. 
      To add a cumulative score, call 'add_aggregate_eval_fn' with a score category, score name and an eval function
      for how to calc the score for a given entity by that metric. For example, see inventory_service.lua.
      The cumulative score can then be read out of the datastore associated with your player's player_data
      as player_data.score_category.total_score/percentage/level
      To break down cumulative scores by level, add level data to score_category_data.json/ 
]]

local ScoreService = class()

function ScoreService:initialize()
   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.player_scores = {}
   end
end

function ScoreService:change_score(entity, category_name, reason, value)
   local player_id = radiant.entities.get_player_id(entity)
   if not player_id or player_id == '' then
      return
   end
   local scores = self:get_scores_for_player(player_id)
   scores:change_score(entity, category_name, reason, value)
end

function ScoreService:get_scores_for_player(player_id)
   local scores = self._sv.player_scores[player_id]
   if not scores then
      scores = radiant.create_controller('stonehearth:score:player_score_data', player_id)
      self._sv.player_scores[player_id] = scores
   end
   return scores
end

return ScoreService