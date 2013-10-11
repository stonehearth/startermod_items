local IdleBoredAction = class()

IdleBoredAction.name = 'run a random effect'
IdleBoredAction.does = 'stonehearth:idle:bored'
IdleBoredAction.priority = 1

function IdleBoredAction:__init(ai, entity)
   self._ai = ai;
   self._entity = entity;
   self._effects = radiant.entities.get_entity_data(entity, 'stonehearth:idle_boredom_effects')
end

function IdleBoredAction:run(ai, entity)
   
   if self._effects then
      local effect = math.random(1, #self._effects)
      ai:execute('stonehearth:run_effect', self._effects[effect])
   end
end

return IdleBoredAction