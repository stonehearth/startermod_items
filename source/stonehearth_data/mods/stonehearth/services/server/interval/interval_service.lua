IntervalService = class()

--[[
   This service runs scenarios that happen at a regular interval. 
   interval_data.json determines which scenarios, what the interval before their first run, 
   and the intervals afterwards. The scenarios can be enabled/disabled/ 

   For example, we get an opportunity to get a new settler once a day or so.

   TODO: we could theoretically move linear combat service onto this, but won't, since that's about to be replaced by goblin camp
]]

function IntervalService:initialize()
   self._sv = self.__saved_variables:get_data()
   if not self._sv._initialized then
      --Do this on first load
      self._sv._initialized = true
      self:_load_data()
      self:_start_all_spawn_timers()
      self.__saved_variables:mark_changed()
   else
      --if we're loading, restore the timer to the next spawn time
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
         self:_restore_spawn_timers()
      end)
   end
end

function IntervalService:_load_data()
   self._sv._data = radiant.resources.load_json('stonehearth:interval_data')
end

-- Start all the spawn timers at the same time
function IntervalService:_start_all_spawn_timers()
   for scenario_name, scenario_data in pairs(self._sv._data.scenarios) do 
      scenario_data.next_spawn_time = self:_start_spawn_timer(scenario_data.time_till_first_occurance, scenario_name, scenario_data.occurance_interval)
   end
   self.__saved_variables:mark_changed()
end

-- Clear the spawn timer, then reset with the duration.
-- Note the expire time for reload useage
-- @param curr_duration - the duration until the scenario is run
-- @param scenario_name - the name of the scenario to run
-- @param next_interval - if the scenario will run in a loop, this is how long between the next run and the subsequent run
function IntervalService:_start_spawn_timer(curr_duration, scenario_name, next_interval)
   local spawn_timer = stonehearth.calendar:set_timer(curr_duration, function()
      --spawn_timer:destroy()
      --spawn_timer = nil
      self._sv._data.scenarios[scenario_name].next_spawn_time = nil
      self:_spawn(scenario_name, next_interval)
   end)
   return spawn_timer:get_expire_time()
end

-- Restart all the spawn timers based on their saved durations from the last run
function IntervalService:_restore_spawn_timers()
   for scenario_name, scenario_data in pairs(self._sv._data.scenarios) do
      local duration = scenario_data.next_spawn_time and stonehearth.calendar:get_seconds_until(scenario_data.next_spawn_time)
      if not duration then
         duration = scenario_data.occurance_interval
      end 
      scenario_data.next_spawn_time = self:_start_spawn_timer(duration, scenario_name, scenario_data.occurance_interval)
   end
   self.__saved_variables:mark_changed()
end

--The timers run regardless of whether the scenario is enabled. 
--Enabling a scenario means that when it is next slotted to occur, it will occur
function IntervalService:enable_scenario(scenario_name, enabled)
   self._sv._data.scenarios[scenario_name].enabled = enabled
end

-- Given the name and the next interval to spawn create the next timer and create the scenario
-- If the scenarios are not currently enabled do not run them
-- @param scenario_name - then scenario we're about to run
-- @param next_interval - time until we next run the scenario
function IntervalService:_spawn(scenario_name, next_interval)
   local next_run = self:_start_spawn_timer(next_interval, scenario_name, next_interval)
   self._sv._data.scenarios[scenario_name].next_spawn_time = next_run
   if self._sv._data.scenarios[scenario_name].enabled then
      stonehearth.dynamic_scenario:force_spawn_scenario(scenario_name)
   end
   self.__saved_variables:mark_changed()
end

return IntervalService