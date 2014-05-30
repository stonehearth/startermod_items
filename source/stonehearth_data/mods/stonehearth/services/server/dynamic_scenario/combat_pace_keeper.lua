local log = radiant.log.create_logger('combat_pace')

local CombatPaceKeeper = class()

function CombatPaceKeeper:initialize()
   -- Start with a large initial value so that we give ourselves some time before ever being
   -- considered for a scenario.
   self._sv._combat_value = 10
   self:restore()
end

function CombatPaceKeeper:restore()
   radiant.events.listen(radiant, 'stonehearth:combat:combat_action', self, self._on_combat_action)
end

function CombatPaceKeeper:get_name()
   return 'combat'
end

function CombatPaceKeeper:get_current_value()
  return self._sv._current_value
end

function CombatPaceKeeper:get_max()
   local military_score = stonehearth.score:get_scores_for_player('player_1'):get_score_data().military_strength
   local military_strength = military_score and military_score.total_score or 0

   -- TODO: this is where combat difficulty will get factored in to the code.
   return (military_strength + 1) * 1.5
end

function CombatPaceKeeper:get_min()
   local military_score = stonehearth.score:get_scores_for_player('player_1'):get_score_data().military_strength
   local military_strength = military_score and military_score.total_score or 0

   -- TODO: this is where combat difficulty will get factored in to the code.
   return military_strength * 0.5
end

function CombatPaceKeeper:get_cooldown_time()
   return 1000
end

function CombatPaceKeeper:decay(value)
  return value * 0.5
end


function CombatPaceKeeper:spawning_scenario(scenario)
   -- TODO probably want some bookkeeping here.
end


function CombatPaceKeeper:_on_combat_action(e)
  -- For now, just add damage done, regardless of who is doing it.  Very shortly, we'll probably
  -- want to weight damage done to the player as more important.
  self._sv._combat_value = self._sv._combat_value + e.damage
  self.__saved_variables:mark_changed()
end

function CombatPaceKeeper:compute_value()
  -- The value of combat pacing is equal to the amount of damage currently inflicted, plus
  -- the value of all of the enemy combat troops.  The idea is that if there are lots of enemy 
  -- troops, but no violence has been done yet, we still want to penalize spawning new combat 
  -- scenarios if there are lots of baddies present that will probably do violence.
  local gm_scores = stonehearth.score:get_scores_for_player('game_master')
  local enemy_pop_size = 0

  local gm_population = stonehearth.population:get_all_populations()['game_master']
  if gm_population then
    for _ in pairs(gm_population:get_citizens()) do enemy_pop_size = enemy_pop_size + 1 end
  end

  local new_combat_value = self._sv._combat_value + enemy_pop_size

  log:spam('Per-tick combat pace value is %d', new_combat_value)
  self._sv._combat_value = 0
  self.__saved_variables:mark_changed()
  return new_combat_value
end

function CombatPaceKeeper:can_spawn_scenario(scenario)
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

return CombatPaceKeeper
