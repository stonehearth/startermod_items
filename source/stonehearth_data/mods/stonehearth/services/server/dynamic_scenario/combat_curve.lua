local log = radiant.log.create_logger('dm_service')

local CombatCurve = class()

CombatCurve.RELAXED = 0
CombatCurve.PEAKED = 1
CombatCurve.COOL_DOWN = 2

function CombatCurve:__init(min_threshold, max_threshold, decay_constant, cooldown_time)
  self._current_value = 0
  self._current_state = CombatCurve.RELAXED
  self._decay_constant = decay_constant
  self._min_threshold = min_threshold
  self._max_threshold = max_threshold
  self._cooldown_time = cooldown_time

  self._combat_value = 0
  radiant.events.listen(radiant, 'stonehearth:combat_stuff', self, self._on_combat_event)
end

function CombatCurve:get_scenario_type()
  return 'COMBAT'
end

function CombatCurve:get_current_value()
  return self._current_value
end

function CombatCurve:get_max()
  return self._max_threshold
end

function CombatCurve:get_min()
  return self._min_threshold
end

function CombatCurve:get_state()
  return self._current_state
end

function CombatCurve:_decay(value)
  return value * self._decay_constant
end

function CombatCurve:update(now)
  -- Accumulate current tick's value with our decayed current value.
  -- This implies that 'compute_value' should always give you the tick's value--a delta,
  -- in other words.  
  self._current_value = self:_decay(self._current_value) + self:compute_value()

  if self._current_state == CombatCurve.RELAXED then
    if self._current_value > self._max_threshold then
      self._current_state = CombatCurve.PEAKED
    end
  elseif self._current_state == CombatCurve.PEAKED then
    if self._current_value < self._min_threshold then
      self._current_state = CombatCurve.COOL_DOWN
      self._cooldown_start = now
    end
  elseif self._current_state == CombatCurve.COOL_DOWN then
    if now - self._cooldown_start > self._cooldown_time then
      self._current_state = CombatCurve.RELAXED
    end
  end
end

function CombatCurve:willing_to_spawn()
  return self:get_state() == CombatCurve.RELAXED
end

-- *********************************************************************************************
-- COMBAT SPECIFIC FUNCTIONS *******************************************************************
-- *********************************************************************************************
function CombatCurve:_on_combat_event(e)
  self._combat_value = self._combat_value -- + f(e)
end

-- API function
function CombatCurve:compute_value()
  local new_combat_value = self._combat_value
  self._combat_value = 0
  return new_combat_value
end

-- API function
function CombatCurve:can_spawn_scenario(scenario)
  local props = scenario.scenario_types.combat

  local combat_strength = 17
  local min_strength = props.min_strength and props.min_strength or 0
  local max_strength = props.max_strength and props.max_strength or 999999

  -- Check strength of the player's army.
  if combat_strength < min_strength or combat_strength >= max_strength then
    return false
  end

  -- Finally, check if the scenario itself has specific run-time spawn requirements.
  return scenario.can_spawn()
end