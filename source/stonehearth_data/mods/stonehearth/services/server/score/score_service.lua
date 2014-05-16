--[[
   Keep track of the scores relevant to different players
   - Whenever the happiness score changes, someone will call the update_town_happiness function
   - every 10 minutes of in-game time, calculate the net worth of the whole civilization
]]
local ScoreService = class()
local PlayerScoreData = require 'services.server.score.player_score_data'

function ScoreService:initialize()
   self._sv = self.__saved_variables:get_data()
   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.player_scores = {}

      -- Set up the net worth timer
      self._net_worth_timer = stonehearth.calendar:set_interval(stonehearth.constants.score.NET_WORTH_INTERVAL, function()
         self:_calculate_net_worth()
      end)
      self._sv.expire_time = self._net_worth_timer:get_expire_time()
   else
      -- create a new datastore for each player, so they can be remoted individually
      for player_id, saved_state in pairs(self._sv.player_scores) do 
         local score_data = PlayerScoreData()
         score_data.__saved_variables = saved_state
         score_data:initialize()
         self._sv.player_scores[player_id] = score_data
      end
      
      --restart the timer on load
      radiant.events.listen(radiant, 'radiant:game_loaded', function(e)
            self:_create_worth_timer()
            return radiant.events.UNLISTEN
         end)
   end
   self._worth_eval_fns = {}
end

--- Save and load the net worth timer accurately.
function ScoreService:_create_worth_timer()
   if self._sv.expire_time then
      --We're still waiting to calculate score, so continue to calculate at the remainder interval
      local duration = self._sv.expire_time - stonehearth.calendar:get_elapsed_time()
      self._timer = stonehearth.calendar:set_timer(duration, function()
            self:_calculate_net_worth()
            if self._sv.expire_time then
               --We're going to grow more so set up the recurring growth timer
               self._net_worth_timer = stonehearth.calendar:set_interval(stonehearth.constants.score.NET_WORTH_INTERVAL, function()
                                    self:_calculate_net_worth()
                                    end)
               self._sv.expire_time = self._net_worth_timer:get_expire_time()
            end
         end)
   end
end

--- Services should call this to add their eval functions for net worth
--  Should be a function that takes an entity and returns a value for that entity 
function ScoreService:add_net_worth_eval_function(score_name, eval_fn)
   self._worth_eval_fns[score_name] = eval_fn
end

--- Calculate the net worth by iterating through entities and summing their values
--  TODO: figure out how to derive faction from player_id
function ScoreService:_calculate_net_worth()
   if self._net_worth_timer then
      self._sv.expire_time = self._net_worth_timer:get_expire_time()
   end

   --Iterate though all the players we know about and add their scores
   for player, score_data in pairs(self._sv.player_scores) do
      score_data:set_net_worth(self:_get_net_worth_for_player(player))
   end
end

--- Get all the things in the area that belong to this player and count them. 
--  If an item is a stockpile, a building, or a placed item, count it. 
function ScoreService:_get_net_worth_for_player(player_id)
   local net_worth_score = {}

   self:_calc_score_from_entities(player_id, net_worth_score)
   --self:_calc_score_from_people(net_worth_score)

   self:_calculate_total_score_from_subscores(net_worth_score)

   return net_worth_score
end

--- Sum the total score, and relevant percentages to next category of score
function ScoreService:_calculate_total_score_from_subscores(net_worth_score)
   local total_score = 0
   for type, score in pairs(net_worth_score) do
      total_score = total_score + score
   end
   total_score =  tonumber(string.format("%.0f", total_score))
        
   net_worth_score.total_score = total_score
   
   local last_cat_threshhold = 0 
   for i, category in ipairs(stonehearth.constants.score.net_worth_categories) do
      local upper_bound = stonehearth.constants.score.category_threshholds[category]
      if total_score < upper_bound then
         net_worth_score.category = category
         net_worth_score.percentage = (total_score - last_cat_threshhold) / (upper_bound - last_cat_threshhold) * 100
         net_worth_score.percentage =  tonumber(string.format("%.1f", net_worth_score.percentage))
        
         if net_worth_score.percentage < 1 then
            net_worth_score.percentage = 1
         end
         break
      end
      last_cat_threshhold = upper_bound
   end
   --We must be beyond the biggest category, so set to the biggest category
   --TODO: Localize category names?
   if not net_worth_score.category and total_score > last_cat_threshhold then
      net_worth_score.category = 'Capitol'
      net_worth_score.percentage = 100
   end
end

---  Iterate through the terrain we've exposed and all items in it. 
function ScoreService:_calc_score_from_entities(player_id, net_worth_score)
   -- TODO: talked to Albert, will map 'player_one' to 'civ' until we have a mechanism of
   -- getting this out of a database
   local faction = '???'
   if player_id == 'player_1' then
      faction = 'civ'
   end
   stonehearth.terrain:get_entities_in_explored_region(faction, function(entity)
      --iterate through the functions and call each with the entity. 
      if not self:_belongs_to_player(entity, player_id) then 
         return false
      else
         for score, eval_fn in pairs(self._worth_eval_fns) do
            if not net_worth_score[score] then
               net_worth_score[score] = 0
            end
            eval_fn(entity, net_worth_score)
         end
      end
   end)
end

--[[
-- START HERE ON THURSDAY!!! TAKE THIS FUNCTION AND MOVE IT TO POPULATION SERVICE
-- itereate through the people in town and assign points for different professions
function ScoreService:_calc_score_from_people(net_worth_score)
   -- TODO: add score values to some professions
   local pop = stonehearth.population:get_population(player_id)
   local citizens = pop:get_citizens()
   for id, citizen in pairs(citizens) do
      local description = citizen:get_component('unit_info'):get_description()
      local alias = 'stonehearth:professions:'.. description
      local data = radiant.resources.load_json(alias)
      if data and data.score_value then
         --TODO: add or multiply?
         net_worth_score.citizens = net_worth_score.citizens + stonehearth.constants.score.DEFAULT_CIV_WORTH + data.score_value
      else
         net_worth_score.citizens = net_worth_score.citizens + stonehearth.constants.score.DEFAULT_CIV_WORTH
      end
   end
end
]]

--- Returns true if the entity is owned by this player, false otherwise
function ScoreService:_belongs_to_player(entity, player_id)
   return radiant.entities.get_player_id(entity) == player_id 
end


--- Call whenever the aggregate happiness for a player should be updated
function ScoreService:update_town_happiness_score(player_id)
   local pop = stonehearth.population:get_population(player_id)
   local citizens = pop:get_citizens()
   
   if not self._sv.player_scores[player_id] then
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
   self._sv.player_scores[player_id]:set_happiness(happiness_score)

   self.__saved_variables:mark_changed()
end


function ScoreService:get_scores_for_player(player_id)
   return self._sv.player_scores[player_id]
end

return ScoreService