local log = radiant.log.create_logger('combat_pace')

local CombatCurve = class()

CombatCurve.RELAXED = 1
CombatCurve.PEAKED = 2
CombatCurve.COOL_DOWN = 3

function CombatCurve:__init(min_threshold_fn, max_threshold_fn, decay_constant_fn, cooldown_time_fn, datastore)
  self.__saved_variables = datastore
  self._sv = datastore:get_data()

  self._decay_constant_fn = decay_constant_fn
  self._min_threshold_fn = min_threshold_fn
  self._max_threshold_fn = max_threshold_fn
  self._cooldown_time_fn = cooldown_time_fn

  if not self._sv._current_state then 
    self._sv._current_value = 0
    self._sv._current_state = CombatCurve.RELAXED

    self._sv._combat_value = 0
  end
  radiant.events.listen(radiant, 'stonehearth:combat:combat_action', self, self._on_combat_action)
end

function CombatCurve:get_scenario_type()
  return 'COMBAT'
end

function CombatCurve:get_current_value()
  return self._sv._current_value
end

function CombatCurve:get_max()
  return self._max_threshold_fn()
end

function CombatCurve:get_min()
  return self._min_threshold_fn()
end

function CombatCurve:get_state()
  return self._sv._current_state
end

function CombatCurve:_decay(value)
  return value * self._decay_constant_fn()
end

function CombatCurve:update(now)
  -- Accumulate current tick's value with our decayed current value.
  -- This implies that 'compute_value' should always give you the tick's value--a delta,
  -- in other words.  
  self._sv._current_value = self:_decay(self._sv._current_value) + self:compute_value()
  log:spam('Total combat pace value is %d, min is %d, max is %d', self._sv._current_value, self:get_min(), self:get_max())

  if self._sv._current_state == CombatCurve.RELAXED then
    if self._sv._current_value > self:get_max() then
      log:spam('Combat pace entering peak.')
      self._sv._current_state = CombatCurve.PEAKED
    end
  elseif self._sv._current_state == CombatCurve.PEAKED then
    if self._sv._current_value < self:get_min() then
      log:spam('Combat pace entering cool-down.')
      self._sv._current_state = CombatCurve.COOL_DOWN
      self._cooldown_start = now
    end
  elseif self._sv._current_state == CombatCurve.COOL_DOWN then
    if now - self._cooldown_start > self._cooldown_time_fn() then
      log:spam('Combat pace entering relaxed.')
      self._sv._current_state = CombatCurve.RELAXED
    end
  end
  self.__saved_variables:mark_changed()
end

function CombatCurve:willing_to_spawn()
  return self:get_state() == CombatCurve.RELAXED
end

-- *********************************************************************************************
-- COMBAT SPECIFIC FUNCTIONS *******************************************************************
-- *********************************************************************************************
function CombatCurve:_on_combat_action(e)
  -- For now, just add damage done, regardless of who is doing it.  Very shortly, we'll probably
  -- want to weight damage done to the player as more important.
  self._sv._combat_value = self._sv._combat_value + e.damage
  self.__saved_variables:mark_changed()
end

-- API function
function CombatCurve:compute_value()
  -- The value of combat pacing is equal to the amount of damage currently inflicted, plus
  -- the value of all of the enemy combat troops.  The idea is that if there are lots of enemy 
  -- troops, but no violence has been done yet, we still want to penalize spawning new combat 
  -- scenarios if there are lots of baddies present that will probably do violence.
  local gm_scores = stonehearth.score:get_scores_for_player('game_master')
  local military_strength = 0
  local enemy_pop_size = 0

  if gm_scores and gm_scores:get_score_data().military_strength then
    local military_score = gm_scores:get_score_data().military_strength
    military_strength = military_score and military_score.total_score or 0
  end

  local gm_population = stonehearth.population:get_all_populations()['game_master']
  if gm_population then
    for _ in pairs(gm_population:get_citizens()) do enemy_pop_size = enemy_pop_size + 1 end
  end

  local new_combat_value = self._sv._combat_value + military_strength + enemy_pop_size

  log:spam('Per-tick combat pace value is %d', new_combat_value)
  self._sv._combat_value = 0
  self.__saved_variables:mark_changed()
  return new_combat_value
end

-- API function
function CombatCurve:can_spawn_scenario(scenario)
  local props = scenario.properties.scenario_types.combat

  local military_score = stonehearth.score:get_scores_for_player('player_1'):get_score_data().military_strength
  local military_strength = military_score ~= nil and military_score.total_score or 0
  local min_strength = props.min_strength and props.min_strength or 0
  local max_strength = props.max_strength and props.max_strength or 999999

  -- Check strength of the player's army.
  if military_strength < min_strength or military_strength >= max_strength then
    return false
  end

  -- Finally, check if the scenario itself has specific run-time spawn requirements.
  return scenario.scenario.can_spawn()
end

return CombatCurve
