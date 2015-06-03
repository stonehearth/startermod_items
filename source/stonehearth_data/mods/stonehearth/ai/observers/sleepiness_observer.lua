--[[
   Counts up from 0 (totally awake) to 30 (exhausted).
   If it's bedtime and we have a bed, sleep. If we hit 30, sleep wherever we are. 
   Sleeping always resets us to a sleepiness of 0. 
]]

local SleepinessObserver = class()

function SleepinessObserver:__init()
   self._sleep_task = nil
end

--Called once on creation
function SleepinessObserver:initialize(entity)
   self._sv.entity = entity
   self._sv.should_be_sleeping = false
   self._sv.hourly_listener = stonehearth.calendar:set_interval("SleepinessObserver on_hourly", '1h', radiant.bind(self, '_on_hourly'))

   -- "equation" : "70 * 0.1 - stamina * 0.05"
end

--Always called. If restore, called after restore.
function SleepinessObserver:activate()
   self._entity = self._sv.entity
   self._attributes_component = self._entity:add_component('stonehearth:attributes')
   self._sleepiness_listener = radiant.events.listen(self._entity, 'stonehearth:attribute_changed:sleepiness', self, self._on_sleepiness_changed)

   --Should we be sleeping? If so, let's do that
   if self._sv.should_be_sleeping then
      self:_start_sleep_task()
   end
end

function SleepinessObserver:destroy()
   self._sv.hourly_listener:destroy()
   self._sv.hourly_listener = nil

   self._sleepiness_listener:destroy()
   self._sleepiness_listener = nil
end

-- Whenever our sleepiness score is changed,
-- check if we should be going to sleep or setting wakeup state
function SleepinessObserver:_on_sleepiness_changed()
   local sleepiness = self._attributes_component:get_attribute('sleepiness')
   if sleepiness >= stonehearth.constants.sleep.TIRED then
      --Chances are this is already started, but if for some reason it isn't, start it
      self:_start_sleep_task()
   elseif sleepiness <= stonehearth.constants.sleep.MIN_SLEEPINESS then
      self:_finish_sleeping()
   end
end

--Every hour, increment our sleepiness, up to a caps
function SleepinessObserver:_on_hourly()
   local sleepiness = self._attributes_component:get_attribute('sleepiness')
   sleepiness = sleepiness + stonehearth.constants.sleep.HOURLY_SLEEPINESS
   if sleepiness > stonehearth.constants.sleep.MAX_SLEEPINESS then
      sleepiness =  stonehearth.constants.sleep.MAX_SLEEPINESS
   end
   self._attributes_component:set_attribute('sleepiness', sleepiness)

   --If it's bedtime and we haven't already slept recently, go sleep
   if sleepiness >= stonehearth.constants.sleep.BEDTIME_THRESHOLD then
      local now = stonehearth.calendar:get_time_and_date()
      local wake_up_time = self._attributes_component:get_attribute('wake_up_time')
      local sleep_duration = self._attributes_component:get_attribute('sleep_duration')
      local bed_time_start = wake_up_time - math.ceil(sleep_duration / 60) + 1;
      if bed_time_start < 0 then
         bed_time_start = bed_time_start + 24
      end
      if now.hour == bed_time_start then
         self:_start_sleep_task()
      end
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