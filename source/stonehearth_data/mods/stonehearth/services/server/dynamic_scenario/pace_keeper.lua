local log = radiant.log.create_logger('pace_keeper')

local PaceKeeper = class()

PaceKeeper.RELAXED = 1
PaceKeeper.PEAKED = 2
PaceKeeper.COOL_DOWN = 3

function PaceKeeper:initialize(controller)
  self._sv.controller = controller
  self._sv._current_value = 0
  self._sv._current_state = PaceKeeper.RELAXED
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
  self._sv._current_value = self:controller():decay(self._sv._current_value) 
      + self:controller():compute_value()

  if self._sv._current_state == PaceKeeper.RELAXED then
    if self._sv._current_value > self:controller():get_max() then
      log:spam('Pace entering peak.')
      self._sv._current_state = PaceKeeper.PEAKED
    end
  elseif self._sv._current_state == PaceKeeper.PEAKED then
    if self._sv._current_value < self:controller():get_min() then
      log:spam('Pace entering cool-down.')
      self._sv._current_state = PaceKeeper.COOL_DOWN
      self._sv._cooldown_start = now
    end
  elseif self._sv._current_state == PaceKeeper.COOL_DOWN then
    if now - self._cooldown_start > self:controller():get_cooldown_time() then
      log:spam('Pace entering relaxed.')
      self._sv._current_state = PaceKeeper.RELAXED
    end
  end
  self.__saved_variables:mark_changed()
end

function PaceKeeper:willing_to_spawn()
  return self:get_state() == PaceKeeper.RELAXED
end

function PaceKeeper:can_spawn_scenario(scenario)
   return self:controller():can_spawn_scenario(scenario)
end

return PaceKeeper
