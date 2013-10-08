local AttackTopAggro = class()

local Point3 = _radiant.csg.Point3
local Vec3 = _radiant.csg.Point3f

AttackTopAggro.name = 'AttackTopAggro!'
AttackTopAggro.does = 'stonehearth:top'
AttackTopAggro.priority = 0

function AttackTopAggro:__init(ai, entity)
   self._entity = entity
   self._ai = ai
   radiant.events.listen('radiant:events:slow_poll', self)
end

AttackTopAggro['radiant:events:slow_poll'] = function(self)
   self._target = radiant.combat.get_target_table_top(self._entity, 'aggro')

   if self._target then
      local priority = radiant.combat.compare_attribute(self._entity, self._target, 'ferocity')
      self._ai:set_action_priority(self, priority)
   else
      self._ai:set_action_priority(self, 0)
   end
end

function AttackTopAggro:run(ai, entity)
   assert(self._target)
   ai:execute('stonehearth:attack', self._target)
end

return AttackTopAggro