local AttackAction = class()

AttackAction.name = 'stonehearth.actions.attack'
AttackAction.does = 'stonehearth.activities.top'
AttackAction.priority = 0

function AttackAction:__init(ai, entity)
   self._entity = entity
   self._ai = ai
   radiant.events.listen('radiant.events.slow_poll', self)
end

AttackAction['radiant.events.slow_poll'] = function(self)
   self._target = radiant.combat.get_target_table_top(self._entity, 'aggro')

   if self._target then
      local priority = radiant.combat.compare_attribute(self._entity, self._target, 'ferocity')
      self._ai:set_action_priority(self, priority)
   else
      self._ai:set_action_priority(self, 0)
   end
end

--- xxx
function AttackAction:run(ai, entity)
   assert(self._target)
   ai:execute('stonehearth.activities.goto_entity', self._target)
   self._ai:set_action_priority(self, 0)
end

return AttackAction