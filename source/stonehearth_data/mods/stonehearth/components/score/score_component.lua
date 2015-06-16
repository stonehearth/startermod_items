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
   end

   --parse the json into scores.
   self:_parse_score_data(json)
end

--- On recieving the initial json, calculate all scores and aggregate scores
function ScoreComponent:_parse_score_data(json)
   --Make score entries for each contributing score type
   for key, data in pairs(json) do
      self:_add_or_update_score_type(key, data)
   end

   --Sort aggregate dependencies topologically. This makes sure we
   --calculate the dependencies in the correct order.
   self._sorted_aggregates = {}
   local all_aggregates = {}
   for aggregate, _ in pairs(self._sv._aggregate_dependencies) do
      local score_data = self._sv.scores[aggregate]
      if score_data.contributes_to then
         table.insert(all_aggregates, aggregate)
      end
   end
   while table.getn(all_aggregates) > 0 do
      local aggregate = table.remove(all_aggregates)
      table.insert(self._sorted_aggregates, aggregate)
      local score_data = self._sv.scores[aggregate]
      if score_data.contributes_to then
         table.insert(all_aggregates, score_data.contributes_to)
      end
   end

   --Now that we've read in all basic scores, 
   --calculate score for each aggregate dependency
   self:_recalculate_all_aggregates()
   self.__saved_variables:mark_changed()
end

--Iterates through the sorted list of aggregates and calls calculate score on each.
function ScoreComponent:_recalculate_all_aggregates()
   for i, aggregate in ipairs(self._sorted_aggregates) do
      local score = self:_calculate_score_for_aggregate(aggregate)
      if not self._sv.scores[aggregate] then 
         local data = {
            starting_score = score
         }
         self:_add_or_update_score_type(aggregate, data)
      else
         self._sv.scores[aggregate].score = score
      end
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
function ScoreComponent:_add_or_update_score_type(key, data)
   if not self._sv.scores[key] then
      self._sv.scores[key] = data
      self._sv.scores[key].score = data.starting_score

      --Assuming that we contribute to a single entity
      --TODO: if more than one, split the string and call once per word in split string 
      if data.contributes_to then
         self:_add_aggregate_dependencies(data.contributes_to, key)
      end
   else
      local old_data = self._sv.scores[key];
      self._sv.scores[key] = data
      self._sv.scores[key].score = old_data.score

      -- If the contributes to field has changed, make sure we update that
      if old_data.contributes_to ~= data.contributes_to then
         if old_data.contributes_to then
            -- Remove the old dependency
            local index = 0
               for i, dep in ipairs (self._sv._aggregate_dependencies[old_data.contributes_to]) do
                  if dep == key then
                     index = i
                     break
                  end
               end
               if index > 0 then
                  table.remove(self._sv._aggregate_dependencies[old_data.contributes_to], index)
               end
         end
         if data.contributes_to then
            self:_add_aggregate_dependencies(data.contributes_to, key)
         end
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
      self:_add_or_update_score_type(key, score_data)
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
         local current_aggregate_score = self._sv.scores[score_data.contributes_to].score
         local aggregate_new_score = self:_calculate_score_for_aggregate(score_data.contributes_to)
         local difference = aggregate_new_score - current_aggregate_score
         self:change_score(score_data.contributes_to, difference)
         --self._sv.scores[score_data.contributes_to].score = self:_calculate_score_for_aggregate(score_data.contributes_to)
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
      self:_add_or_update_score_type(key, score_data)
   end
   return score_data.score
end

--- Given a key, get the score associated with it and return it
function ScoreComponent:has_score(key)
   return self._sv.scores[key] ~= nil
end

function ScoreComponent:_add_score_event(path, value, journal_type)
--Make the new score buckets if it doesn't exist
end

function ScoreComponent:_change_eval_function(uri)
--TODO: workers and fighters should have differnt logic for evaluating their score. 
--Though aggression events may always adjust protectedness, the value might be different for different people
--look into carpetner promote/demote, etc
end

-- Used for cheats. Iterates through all the scores and resets them to
-- their default values.
function ScoreComponent:reset_all_scores()
   for score_name, score_data in pairs(self._sv.scores) do
      score_data.score = score_data.starting_score
   end
   self:_recalculate_all_aggregates()
   self.__saved_variables:mark_changed()
end

return ScoreComponent