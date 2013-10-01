local IdleDoEffectAction = class()

IdleDoEffectAction.name = 'run a random effect'
IdleDoEffectAction.does = 'stonehearth.idle.bored'
IdleDoEffectAction.priority = 1

function IdleDoEffectAction:__init(ai, entity)
   self._ai = ai;
   self._entity = entity;
   self._effects = radiant.entities.get_entity_data(entity, 'stonehearth:idle_boredom_effects')
end

function IdleDoEffectAction:run(ai, entity)
   
   if self._effects then
      local effect = math.random(1, #self._effects)
      ai:execute('stonehearth.run_effect', self._effects[effect])
   end
end

return IdleDoEffectAction