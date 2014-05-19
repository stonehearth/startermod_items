--[[
   Put on any entity who you want to track "score" for.

   Scores may be aggregates (in which case, they're averages of their child scores) or direct scores which are
   changed by various observers, etc. 
]]

local ScoreComponent = class()

function ScoreComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()

   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.scores = {}
      self._sv._aggregate_dependencies = {}

      --parse the json into scores.
      self:_parse_initial_score_data(json)
   end
end

--- On recieving the initial json, calculate all scores and aggregate scores
function ScoreComponent:_parse_initial_score_data(json)
   --Make score entries for each contributing score type
   for key, data in pairs(json) do
      self:_add_new_score_type(key, data)
   end

   --Now that we've read in all basic scores, 
   --calculate score for each aggregate dependency
   for aggregate, dependencies in pairs(self._sv._aggregate_dependencies) do
      local data = {
         starting_score = self:_calculate_score_for_aggregate(aggregate)
      }
      self:_add_new_score_type(aggregate, data)
   end
end

--Given a score name, see if it is built of out any dependencies. If so, average them by
--stated weight to get the default score for that aggregate
function ScoreComponent:_calculate_score_for_aggregate(name)
   local dependencies = self._sv._aggregate_dependencies[name]
   assert (#dependencies > 0, 'problem: no dependencies for this aggregate score')
   local sum = 0
   for i, dependency in pairs(dependencies) do
      sum = sum + self._sv.scores[dependency].score * self._sv.scores[dependency].weight
   end
   sum = sum / #dependencies
   return sum
end

--- Add a new score by its associated data
--  @param key - the name of the score
--  @param data - json about the score, which should incude starting score,
--                weight, and whether it contributes to any aggregates, if any
function ScoreComponent:_add_new_score_type(key, data) 
   if not self._sv.scores[key] then
      self._sv.scores[key] = data
      self._sv.scores[key].score = data.starting_score

      --Assuming that we contribute to a single entity
      --TODO: if more than one, split the string and call once per word in split string 
      if data.contributes_to then
         self:_add_aggregate_dependencies(data.contributes_to, key)
      end
   end
end

--- Given an aggregate type and a score that contributes to it, set up the data in the dependency table
function ScoreComponent:_add_aggregate_dependencies(aggregate_name, dependency_name)
   local aggregate_data = self._sv._aggregate_dependencies[aggregate_name]
   if not aggregate_data then
      --start a new entry
      self._sv._aggregate_dependencies[aggregate_name] = {dependency_name}
   else
      --add the new dependency to the entry
      table.insert(aggregate_data, dependency_name)
   end
end

--- Given a key and a modifier, update the score at that key
--  @param key - the name of the score to update
--  @param modifier - the number to add to the score associated with the key
--  @param journal_data - a bag of stuff to pass to the journal entry associated with this score
function ScoreComponent:change_score(key, modifier, journal_data)
   local score_data = self._sv.scores[key]

   --If the score data doesn't exist, make something up
   if not score_data then
      score_data = {
         starting_score = stonehearth.constants.score.DEFAULT_VALUE + modifier
      }
      self:_add_new_score_type(key, score_data)
   else
      --If the score data does exist, 
      score_data.score = score_data.score + modifier
      
      --Adjust for max and min
      local max = score_data.max
      local min = score_data.min
      if not max then
         max = stonehearth.constants.score.DEFAULT_MAX
      end
      if not min then 
         min = stonehearth.constants.score.DEFAULT_MIN
      end
      if score_data.score > max then
         score_data.score = max
      elseif score_data.score < min then
         score_data.score = min
      end

      --Change dependent scores
      if score_data.contributes_to then
         --TODO: accomodate more than 1 score
         self._sv.scores[score_data.contributes_to].score = self:_calculate_score_for_aggregate(score_data.contributes_to)
      end
   end

   --Since the score should trigger a journal entry, have that pop in
   --If every score-related thing is going to prompt a journal entry, make an assert instead of an if statement
   if journal_data then
      local score_metadata = {score_name = key, score_mod = modifier}
      stonehearth.personality:log_journal_entry(journal_data, score_metadata)   
   end


   --Mostly for autotests, trigger that the score has changed
   radiant.events.trigger_async(self._entity, 'stonehearth:score_changed', {
         key = key, 
         modifier = modifier, 
         new_score = score_data.score
      })
   
   --Tell the score service that the score has changed
   local player_id = radiant.entities.get_player_id(self._entity)
   if player_id then
      stonehearth.score:update_aggregate_score(player_id)
   end

   self.__saved_variables:mark_changed()
end

function ScoreComponent:get_all_scores()
   return self._sv.scores
end

--- Given a key, get the score associated with it and return it
function ScoreComponent:get_score(key)
   local score_data = self._sv.scores[key]
   if not score_data then
      score_data = {
         starting_score = stonehearth.constants.score.DEFAULT
      }
      self:_add_new_score_type(key, score_data)
   end
   return score_data.score
end

function ScoreComponent:_add_score_event(path, value, journal_type)
--Make the new score buckets if it doesn't exist
end

function ScoreComponent:_change_eval_function(uri)
--TODO: workers and fighters should have differnt logic for evaluating their score. 
--Though aggression events may always adjust protectedness, the value might be different for different people
--look into carpetner promote/demote, etc
end


return ScoreComponent