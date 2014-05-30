local log = radiant.log.create_logger('pace_keeper')

local PaceKeeper = class()

PaceKeeper.RELAXED = 1
PaceKeeper.PEAKED = 2
PaceKeeper.COOL_DOWN = 3

function PaceKeeper:initialize(controller)
   self._sv.controller = controller
   self._sv._current_value = 0
   self._sv._current_state = PaceKeeper.RELAXED
   self._sv._buildup = 0
end

function PaceKeeper:restore()

end


function PaceKeeper:controller()
   return self._sv.controller
end


function PaceKeeper:get_state()
   return self._sv._current_state
end


function PaceKeeper:update(now)
   -- Accumulate current tick's value with our decayed current value.
   -- This implies that 'compute_value' should always give you the tick's value--a delta,
   -- in other words.  
   self._sv._current_value = math.floor(self:controller():decay(self._sv._current_value) 
      + self:controller():compute_value())

   local name = self:controller():get_name()

   log:spam('%s pace value is %d', name, self._sv._current_value)

   if self._sv._current_state == PaceKeeper.RELAXED then
      if self._sv._current_value > self:controller():get_max() then
         log:spam('%s pace entering peak.', name)
         self._sv._current_state = PaceKeeper.PEAKED
      end
   elseif self._sv._current_state == PaceKeeper.PEAKED then
      if self._sv._current_value <= self:controller():get_min() then
         log:spam('%s pace entering cool-down.', name)
         self._sv._current_state = PaceKeeper.COOL_DOWN
         self._sv._cooldown_start = now
      end
   elseif self._sv._current_state == PaceKeeper.COOL_DOWN then
      if now - self._sv._cooldown_start > self:controller():get_cooldown_time() then
         log:spam('%s pace entering relaxed.', name)
         self._sv._current_state = PaceKeeper.RELAXED
      end
   end

   self._sv._buildup = self._sv._buildup + 1

   self.__saved_variables:mark_changed()
end

function PaceKeeper:get_buildup_value()
   return self._sv._buildup
end

function PaceKeeper:willing_to_spawn()
   return self:get_state() == PaceKeeper.RELAXED
end

function PaceKeeper:is_valid_scenario(scenario)
   return self:controller():can_spawn_scenario(scenario)
end

function PaceKeeper:spawning_scenario(scenario)
   self:controller():spawning_scenario(scenario)
end

function PaceKeeper:clear_buildup()
   self._sv._buildup = 0
   self.__saved_variables:mark_changed()
end

return PaceKeeper
