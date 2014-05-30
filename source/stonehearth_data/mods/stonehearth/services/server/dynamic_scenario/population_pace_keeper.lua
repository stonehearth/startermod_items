nlocal log = radiant.log.create_logger('combat_pace')

local PopulationPaceKeeper = class()

function PopulationPaceKeeper:initialize()
   self._sv._pop_value = 0
end


function PopulationPaceKeeper:restore()
end


function PopulationPaceKeeper:get_current_value()
   return self._sv._pop_value
end


function PopulationPaceKeeper:get_max()
   return 100
end


function PopulationPaceKeeper:get_min()
   return 0
end

function PopulationPaceKeeper:get_name()
   return 'population'
end


function PopulationPaceKeeper:get_cooldown_time()
   return (24 * 60 * 60) / 9  -- 9 ticks per second
end


function PopulationPaceKeeper:decay(value)
   -- We immediately reset ourselves to ground after spawning something, but
   -- the cooldown ensures that we only spawn once a day.
   return 0.0
end

function PopulationPaceKeeper:spawning_scenario(scenario)
   -- Idea: send the pace-keeper immediately into 'peaked' when spawning.  Our decay
   -- then brings us back to baseline immediately, and then the cooldown_timer gives
   -- us a day until we can spawn something again.
   self._sv._pop_value = 101
end

function PopulationPaceKeeper:compute_value()
   local new_pop_value = self._sv._pop_value
   self._sv._pop_value = 0
   self.__saved_variables:mark_changed()
   return new_pop_value
end

function PopulationPaceKeeper:can_spawn_scenario(scenario)
   return scenario.scenario.can_spawn()
end

return PopulationPaceKeeper
