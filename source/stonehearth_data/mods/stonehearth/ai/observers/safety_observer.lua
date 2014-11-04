local SafetyScoreObserver = class()

--[[ @markdown
-- Safety starts at 5
-- Safety goes down once per day when a unit is cowering/fleeing from enemies
-- Safety goes up when a whole day passes without cowering/fleeing
-- Safety goes down if HP is below 25%
-- Safety goes down when someone else in town is killed

-- TODO?: Safety goes up when you go to sleep and there are footmen in town
-- TODO?: Safety goes up when an enemy is killed by someone from the town (based on their menace) 
--        But how do you know that your dudes killed him? 

-- TODO: consider having this observer change based on class (different observers for combat classes?)
-- TODO: more logs by personality for peaceful_day, hurt_badly, and villager_death
]]

-- May be defunct?
function SafetyScoreObserver:__init(entity)
end

-- Create listeners and initial state
function SafetyScoreObserver:initialize(entity)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()

   self._score_component = entity:add_component('stonehearth:score')
   self._attributes_component = entity:add_component('stonehearth:attributes')

   self._panic_listener = radiant.events.listen(entity, 'stonehearth:panicking', self, self._on_panic)
   self._sv.panic_today = false

   self._combat_battery_listener = radiant.events.listen(entity, 'stonehearth:combat:battery', self, self._on_combat_battery)
   self._sv.dangerously_wounded_today = false

   self._midnight_listener = radiant.events.listen(stonehearth.calendar, 'stonehearth:midnight', self, self._on_midnight)

   self._kill_event_listener = radiant.events.listen(radiant.entities, 'stonehearth:entity_killed', self, self._on_entity_killed )
   self._sv.friend_died = false
end

-- On destroy, nuke the listeners
function SafetyScoreObserver:destroy()
   self._panic_listener:destroy()
   self._panic_listener = nil

   self._midnight_listener:destroy()
   self._midnight_listener = nil

   self._combat_battery_listener:destroy()
   self._combat_battery_listener = nil

   self._kill_event_listener:destroy()
   self._kill_event_listener = nil
end

-- Once a day, when we're panicking, lower our safety score
function SafetyScoreObserver:_on_panic(e)
   if not self._sv.panic_today then
      self._sv.panic_today = true
      local journal_data = {entity = self._entity, description = 'generic_panic', tags = {'safety', 'combat', 'panic'}}
      self._score_component:change_score('safety', stonehearth.constants.score.safety.PANIC_PENALTY, journal_data)
   end
end

-- Once a day, if we fall below 25% of max health, lower our safety score
function SafetyScoreObserver:_on_combat_battery()
   local curr_health = self._attributes_component:get_attribute('health')
   local max_health = self._attributes_component:get_attribute('max_health')
   local percent_health = curr_health/max_health * 100

   if not self._sv.dangerously_wounded_today and percent_health < 25 then
      self._sv.dangerously_wounded_today = true
      local journal_data = {entity = self._entity, description = 'hurt_badly', tags = {'safety', 'combat', 'low_hp'}}
      self._score_component:change_score('safety', stonehearth.constants.score.safety.NEAR_DEATH_PENALTY, journal_data)
   end
end

--If an entity from our population was killed, lower safety score
function SafetyScoreObserver:_on_entity_killed(e)
   if e.player_id and e.player_id == radiant.entities.get_player_id(self._entity) and 
      e.player_id == radiant.entities.get_player_id(self._entity) then

      --Pass in name of the dead friend into the journal entry
      local personality_component = self._entity:get_component('stonehearth:personality')
      local name_break = string.find(e.name, ' ')
      local first_name = string.sub(e.name, 1, name_break-1)
      personality_component:add_substitution('dead_friend', first_name)

      local journal_data = {entity = self._entity, description = 'villager_death', tags = {'safety', 'combat', 'villager_death'}}
      self._score_component:change_score('safety', stonehearth.constants.score.safety.TOWN_DEATH, journal_data)

      --Keep track of this so we know whether we can run the "no activity today" perk, otherwise, can trigger multiple times each day
      self._sv.friend_died = true
   end
end

-- At midnight, if no combat/bad things have happened, raise safety score
function SafetyScoreObserver:_on_midnight()
   --add other variables; if none of them fired, THEN do peaceful day
   if not self._sv.panic_today and not self._sv.dangerously_wounded_today and not self._sv.friend_died then
      local journal_data = {entity = self._entity, description = 'peaceful_day', tags = {'safety', 'improvement'}}
      self._score_component:change_score('safety', stonehearth.constants.score.safety.PEACEFUL_DAY_BONUS, journal_data)
   end
   self._sv.panic_today = false
   self._sv.dangerously_wounded_today = false
   self._sv.friend_died = false
end

return SafetyScoreObserver