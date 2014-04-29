--[[
   Counts up from 0 (totally awake) to 30 (exhausted).
   If it's bedtime and we have a bed, sleep. If we hit 30, sleep wherever we are. 
   Sleeping always resets us to a sleepiness of 0. 
]]

local calendar = stonehearth.calendar

local SleepinessObserver = class()

--TODO: now defunct?
function SleepinessObserver:__init(entity)
end

function SleepinessObserver:initialize(entity, json)
   self._entity = entity
   self._sleep_task = nil
   self._attributes_component = entity:add_component('stonehearth:attributes')

   self._sv = self.__saved_variables:get_data()
   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.should_be_sleeping = false
   end

   radiant.events.listen(calendar, 'stonehearth:hourly', self, self._on_hourly)
   radiant.events.listen(self._entity, 'stonehearth:attribute_changed:sleepiness', self, self._on_sleepiness_changed)

   --Should we be sleeping? If so, let's do that
   if self._sv.should_be_sleeping then
      --TODO: cut sleep up into multiple tasks so we can skip straight to the ZZZ part
      --Or, pass a 'has yawned' flag or something
      self:_start_sleep_task()
   end
end

function SleepinessObserver:destroy()
   radiant.events.unlisten(calendar, 'stonehearth:hourly', self, self._on_hourly)
   radiant.events.unlisten(self._entity, 'stonehearth:attribute_changed:sleepiness', self, self._on_sleepiness_changed)
end

-- Whenever our sleepiness score is changed,
-- check if we should be going to sleep or setting wakeup state
function SleepinessObserver:_on_sleepiness_changed()
   local sleepiness = self._attributes_component:get_attribute('sleepiness')
   if sleepiness >= stonehearth.constants.sleep.EXHAUSTION then
      --Chances are this is already started, but if for some reason it isn't, start it
      self:_start_sleep_task()
   elseif sleepiness <= stonehearth.constants.sleep.MIN_SLEEPINESS then
      self:_finish_sleeping()
   end
end

--Every hour, increment our sleepiness, up to a caps
function SleepinessObserver:_on_hourly(e)
   local sleepiness = self._attributes_component:get_attribute('sleepiness')
   sleepiness = sleepiness + stonehearth.constants.sleep.HOURLY_SLEEPINESS
   if sleepiness > stonehearth.constants.sleep.MAX_SLEEPINESS then
      sleepiness =  stonehearth.constants.sleep.MAX_SLEEPINESS
   end
   self._attributes_component:set_attribute('sleepiness', sleepiness)

   --If it's bedtime, go sleep
   if e.now.hour == stonehearth.constants.sleep.BEDTIME_START then
      self:_start_sleep_task()
   end
end

--Create the sleeping task
function SleepinessObserver:_start_sleep_task()
   --TODO: do we want a sleepy toast?
   --TODO: start a different task if we're loading/pass a no-yawn flag?
   if not self._sleep_task then
      self._sleep_task = self._entity:get_component('stonehearth:ai')
                                       :get_task_group('stonehearth:basic_needs')
                                          :create_task('stonehearth:sleep', {})
                                          :set_priority(stonehearth.constants.priorities.basic_needs.SLEEP)
                                          :start()
      self._sv.should_be_sleeping = true
      self.__saved_variables:mark_changed()
   end
end

-- Destroy the sleep task so we can do other things
function SleepinessObserver:_finish_sleeping()
   self._sv.should_be_sleeping = false
   self.__saved_variables:mark_changed()

   if self._sleep_task then
      self._sleep_task:destroy()
      self._sleep_task = nil
   end
end


return SleepinessObserver