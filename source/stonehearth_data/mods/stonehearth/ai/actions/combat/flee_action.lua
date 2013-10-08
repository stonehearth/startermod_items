local Flee = class()

local Point3 = _radiant.csg.Point3
local Vec3 = _radiant.csg.Point3f

Flee.name = 'stonehearth:actions:flee'
Flee.does = 'stonehearth:top'
Flee.priority = 0

function Flee:__init(ai, entity)
   self._entity = entity
   self._ai = ai
   radiant.events.listen('radiant:events:slow_poll', self)
end

Flee['radiant:events:slow_poll'] = function(self)
   self._target = radiant.combat.get_target_table_top(self._entity, 'aggro')

   if self._target then
      local priority = -10 * radiant.combat.compare_attribute(self._entity, self._target, 'ferocity')
      self._ai:set_action_priority(self, priority)
   else
      self._ai:set_action_priority(self, 0)
   end
end

function Flee:run(ai, entity)
   assert(self._target)
   radiant.entities.set_posture(entity, 'panic')

   while true do
      ai:execute('stonehearth:run_away_from_entity', self._target)
   end
end

function Flee:stop(ai, entity)
   radiant.entities.unset_posture(entity, 'panic')
end
return Flee