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
   local chosen_effect = self:_get_random_effect(idle_effects.effects)
   if chosen_effect then
      ai:execute('stonehearth:run_effect', { effect = chosen_effect.name })
   else
      ai:execute('stonehearth:run_effect', { 'idle_breathe' })
   end
end

function IdleWeighted:_get_random_effect(idle_effects_list)
   local total_weight = 0
   for i, effect in ipairs (idle_effects_list) do
      total_weight = total_weight + effect.weight
   end

   local random_value = _radiant.csg.get_default_rng():get_int(1, total_weight)

   local count = 0
   for i, effect in ipairs (idle_effects_list) do
      count = count + effect.weight
      if random_value <= count then
         return effect
      end
   end
   return nil
end

return IdleWeighted
