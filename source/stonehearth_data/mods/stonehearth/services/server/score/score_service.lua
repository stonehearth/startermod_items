--[[
   Keep track of the scores relevant to different players

   There are a couple kinds of scores. 
   
   1. Scores aggregated from citizens 
      If citizens have fed/happiness/etc scores (incremented by observers and collected on a per-citizen 
      basis by a score_component) we count them all up and present a town-level summary
      for a given player. To add an aggregate scoare, specify it a .json file that belongs to the citizen 
      (for example, see base_human.json) and then manipulate it via observer. 
      Whenever the citizen-related score changes, someone will call the update_citizen_scores function
   
   2. Cumulative scores for the whole town. (For example, net worth for the inventory for the whole town, etc.)
      Every 10 minutes of in-game time, we calculate the cumulative scores. 
      To add a cumulative score, call 'add_aggregate_eval_fn' with a score category, score name and an eval function
      for how to calc the score for a given entity by that metric. For example, see inventory_service.lua.
      The cumulative score can then be read out of the datastore associated with your player's player_data
      as player_data.score_category.total_score/percentage/level
      To break down cumulative scores by level, add level data to score_category_data.json/ 
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

   --Init the score data
   self._aggregate_score_data = {}
   self:_setup_score_category_data()

   --By default, scores for entities in a category are 1. For increased scores
   --check against the score_entity_data table. 
   self._score_entity_table = radiant.resources.load_json('stonehearth:score_entity_data')
end

--- Services should call this to add their eval functions for net worth
--  Should be a function that takes an entity and returns a value for that entity 
--  @param score category - name of the category for this aggregate score (for example, net worth)
--  @param score_name - name for the score (ie, citizens)
--  @param eval_fn - a function that takes an entity and a bag of data, and that updates the bag with the new score
function ScoreService:add_aggregate_eval_function(score_category, score_name, eval_fn)
   self._aggregate_score_data[score_name] = {
      eval_fn = eval_fn, 
      score_category = score_category
   }
end

--- Given the URI of an entity, get the score for it. 
--  If we don't have a score for that entity, use the default score, which is 
--  defined in the file (usually, 1)
function ScoreService:get_score_for_entity_type(entity_uri)
   local score_for_item = self._score_entity_table[entity_uri]
   if not score_for_item then
      score_for_item =  self._score_entity_table['default']
   end
   return score_for_item
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

--- Calculate the net worth by iterating through entities and summing their values
--  TODO: figure out how to derive faction from player_id
function ScoreService:_calculate_net_worth()
   if self._net_worth_timer then
      self._sv.expire_time = self._net_worth_timer:get_expire_time()
   end

   --Iterate though all the players we know about and add their scores
   for player, score_data in pairs(self._sv.player_scores) do
      local aggregate_score_data = self:_get_aggregate_score_for_player(player)
      for score_category, data in pairs(aggregate_score_data) do
         score_data:set_score_type(score_category, data)
      end
   end
end

--- Transform the json score categories into a sorted array for calculations later
function ScoreService:_setup_score_category_data()
   self._score_category_data = radiant.resources.load_json('stonehearth:score_category_data')
   for category_name, category_data in pairs(self._score_category_data) do
      local ordered_levels= {}
      local index = 1
      for level_name, value in pairs(category_data.levels) do
         ordered_levels[index] = {level_name, value}
         index = index + 1
      end
      table.sort(ordered_levels, function(a, b)
            return a[2] < b[2]
         end)
      category_data.ordered_levels = ordered_levels
   end
end


--- Get all the things in the area that belong to this player and this type of score and count them. 
--  If an item is a stockpile, a building, or a placed item, count it. 
function ScoreService:_get_aggregate_score_for_player(player_id)
   local all_agg_scores = {}

   self:_calc_score_from_entities(player_id, all_agg_scores)

   for score_category, score_data in pairs(all_agg_scores) do
      self:_calculate_category_score_from_subscores(score_category, score_data)
   end

   return all_agg_scores
end

--- Sum the total score for the category
--  Optionally, note relevant percentages to next category of score
function ScoreService:_calculate_category_score_from_subscores(score_category, score_data)
   local total_score = 0
   for type, score in pairs(score_data) do
      total_score = total_score + score
   end
   total_score =  tonumber(string.format("%.0f", total_score))
        
   score_data.total_score = total_score

   --Do we want to add data about which category this fits into? If so
   --use the data read in earlier
   if self._score_category_data[score_category] then
      self:_calculate_category_level(score_category, score_data)
   end
end

function ScoreService:_calculate_category_level(score_category, score_data)
   local last_lvl_threshold = 0
   for i, level in ipairs(self._score_category_data[score_category].ordered_levels) do
      local upper_bound = level[2]
      if score_data.total_score < upper_bound then
         score_data.level = level[1]
         score_data.percentage = (score_data.total_score - last_lvl_threshold) / (upper_bound - last_lvl_threshold) * 100
         score_data.percentage =  tonumber(string.format("%.1f", score_data.percentage))
        
         if score_data.percentage < 1 then
            score_data.percentage = 1
         end
         break
      end
      last_lvl_threshold = upper_bound
   end

   --We must be beyond the biggest category, so set to the biggest category
   if not score_data.level and total_score > last_lvl_threshold then
      local num_levels = #self._score_category_data[score_category].ordered_levels
      local last_level = self._score_category_data[score_category].ordered_levels[num_levels][1]
      score_data.percentage = 100
   end
end

---  Iterate through the terrain we've exposed and all items in it. 
function ScoreService:_calc_score_from_entities(player_id, all_agg_scores)
   -- TODO: talked to Albert, will map 'player_one' to 'civ' until we have a mechanism of
   -- getting this out of a database
   local faction = '???'
   if player_id == 'player_1' then
      faction = 'civ'
   end
   stonehearth.terrain:get_entities_in_explored_region(faction, function(entity)
      --iterate through the functions and call each with the entity. 
      if not radiant.entities.is_owned_by_player(entity, player_id) then 
         return false
      else
         for score_name, score_data in pairs(self._aggregate_score_data) do
            local score_category = score_data.score_category
            local eval_fn = score_data.eval_fn
            local agg_score_bag = all_agg_scores[score_category] 

            if not agg_score_bag then
               agg_score_bag = {}
            end

            if not agg_score_bag[score_name] then
               agg_score_bag[score_name] = 0
            end

            eval_fn(entity, agg_score_bag)
            
            all_agg_scores[score_category] = agg_score_bag
         end
      end
   end)
end

--- Call whenever the aggregate happiness for a player should be updated
function ScoreService:update_aggregate_score(player_id)
   local pop = stonehearth.population:get_population(player_id)
   local citizens = pop:get_citizens()
   
   if not self._sv.player_scores[player_id] then
      self._sv.player_scores[player_id] = PlayerScoreData()
      self._sv.player_scores[player_id].__saved_variables = radiant.create_datastore()
      self._sv.player_scores[player_id]:initialize()
   end

   self:_calculate_aggregate(player_id, citizens)
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
function ScoreService:_calculate_aggregate(player_id, citizens)
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
   --TODO: rename this aggregate and change the town UI
   self._sv.player_scores[player_id]:set_score_type('happiness', happiness_score)

   self.__saved_variables:mark_changed()
end


function ScoreService:get_scores_for_player(player_id)
   return self._sv.player_scores[player_id]
end

return ScoreService