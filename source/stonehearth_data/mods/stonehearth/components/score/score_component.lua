--[[
   Put on any entity who you want to track "score" for.

   Scores may be aggregates (in which case, they're averages of their child scores) or direct scores which are
   changed by various observers, etc. 
]]

local ScoreComponent = class()

function ScoreComponent:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()

   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.entity = entity
      self._sv.scores = {}
      self._sv._contributes_to = {}
   end

   --parse the json into scores.
   self:_parse_score_data(json)
end

function ScoreComponent:activate()
   self._player_id_listener = radiant.events.listen(self._sv.entity, 'radiant:unit_info:player_id_changed', self, self._on_player_id_changed)
end

function ScoreComponent:destroy()
   if self._player_id_listener then
      self._player_id_listener:destroy()
      self._player_id_listener = nil
   end
end

function ScoreComponent:_on_player_id_changed(e)
   for category, score_data in pairs(self._sv.scores) do
      stonehearth.score:change_score(self._sv.entity, category, 'score component', score_data.score)
   end
end

--- On recieving the initial json, calculate all scores and aggregate scores
function ScoreComponent:_parse_score_data(json)
   --Make score entries for each contributing score type
   for key, data in pairs(json) do
      self:_get_score_data(key, data)
   end
   self.__saved_variables:mark_changed()
end

--Given a score name, see if it is built of out any dependencies. If so, average them by
--stated weight to get the default score for that aggregate
function ScoreComponent:_calculate_score_for_aggregate(name)
   local dependencies = self._sv._contributes_to[name]
   assert (#dependencies > 0, 'problem: no dependencies for this aggregate score')
   local sum = 0
   local weight_sum = 0
   for i, dependency in pairs(dependencies) do
      sum = sum + self._sv.scores[dependency].score * self._sv.scores[dependency].weight
      weight_sum = weight_sum + self._sv.scores[dependency].weight
   end
   sum = sum / weight_sum
   return sum
end

--- Add a new score by its associated data
--  @param key - the name of the score
--  @param data - json about the score, which should incude starting score,
--                weight, and whether it contributes to any aggregates, if any
function ScoreComponent:_get_score_data(key, score_data)
   local sd = self._sv.scores[key]
   if self._sv.scores[key] then
      return sd
   end

   -- not here?  initialize a new one
   if not score_data then
      score_data = {      
         starting_score = stonehearth.constants.score.DEFAULT_VALUE
      }
   end

   if not score_data.max then
      score_data.max = stonehearth.constants.score.DEFAULT_MAX
   end
   if not score_data.min then 
      score_data.min = stonehearth.constants.score.DEFAULT_MIN
   end   

   self._sv.scores[key] = score_data
   if score_data.contributes_to then
      self:_add_contributes_to(score_data.contributes_to, key)
   end

   -- This will take care of updating the aggregate dependencies
   self:_set_score(key, score_data.starting_score)

   return score_data
end

--- Given an aggregate type and a score that contributes to it, set up the data in the dependency table
function ScoreComponent:_add_contributes_to(aggregate_name, dependency_name)
   local aggregate_data = self._sv._contributes_to[aggregate_name]
   if not aggregate_data then
      --start a new entry
      self._sv._contributes_to[aggregate_name] = { dependency_name }
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
   local score_data = self:_get_score_data(key)
   self:_set_score(key, score_data.score + modifier)

   --Since the score should trigger a journal entry, have that pop in
   --If every score-related thing is going to prompt a journal entry, make an assert instead of an if statement
   if journal_data then
      local score_metadata = {score_name = key, score_mod = modifier}
      stonehearth.personality:log_journal_entry(journal_data, score_metadata)   
   end

   --Mostly for autotests, trigger that the score has changed
   radiant.events.trigger_async(self._sv.entity, 'stonehearth:score_changed', {
         key = key,
         new_score = score_data.score,
      })

   self.__saved_variables:mark_changed()
end

--- Updates the score of the given key to the specified value
--- Then, ensure value is between min and max and update dependencies
--- if key contributes to another score.
--  @param key - the name of the score to update
--  @param value - the value to set the score to
function ScoreComponent:_set_score(key, value)
   local score_data = self:_get_score_data(key)
   assert(score_data)

   score_data.score = math.min(math.max(value, score_data.min), score_data.max)
   stonehearth.score:change_score(self._sv.entity, key, 'score component', value)
   
   -- Change dependent scores
   local aggregate = score_data.contributes_to
   if aggregate then
      local new_score = self:_calculate_score_for_aggregate(aggregate)
      self:_set_score(aggregate, new_score)
   end
end

--- Given a key, get the score associated with it and return it
function ScoreComponent:get_score(key, dflt)
   local score_data = self._sv.scores[key]
   if not score_data then
      return dflt
   end
   return score_data.score
end

-- Used for cheats. Iterates through all the scores and resets them to
-- their default values.
function ScoreComponent:reset_all_scores()
   for score_name, score_data in pairs(self._sv.scores) do
      self:_set_score(score_name, score_data.starting_score)
   end
   self.__saved_variables:mark_changed()
end

return ScoreComponent