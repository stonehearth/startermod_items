local WeightedSet = require 'lib.algorithms.weighted_set'
local rng = _radiant.csg.get_default_rng()

local IdleWeighted = class()

IdleWeighted.name = 'idle'
IdleWeighted.does = 'stonehearth:idle'
IdleWeighted.args = { 
   hold_position = {    -- is the unit allowed to move around in the action?
      type = 'boolean',
      default = false,
   }
}
IdleWeighted.version = 2
IdleWeighted.priority = 1
IdleWeighted.weight = 1

function IdleWeighted:start_thinking(ai, entity)
   if ai.CURRENT.carrying then
      return
   end
   local idle_effects = radiant.entities.get_entity_data(entity, 'stonehearth:idle_effects')
   if not idle_effects then
      return
   end

   ai:set_think_output()
end

function IdleWeighted:run(ai, entity)

   local idle_effects = radiant.entities.get_entity_data(entity, 'stonehearth:idle_effects')
   local effect_name = self:_get_random_effect(idle_effects.effects)
   if effect_name then
      ai:execute('stonehearth:run_effect', { effect = effect_name })
   else
      ai:execute('stonehearth:run_effect', { 'idle_breathe' })
   end
end

function IdleWeighted:_get_random_effect(idle_effects_list)
   local weighted_set = WeightedSet(rng)

   for _, effect in pairs(idle_effects_list) do
      weighted_set:add(effect.name, effect.weight)
   end

   local effect_name = weighted_set:choose_random()
   return effect_name
end

return IdleWeighted
