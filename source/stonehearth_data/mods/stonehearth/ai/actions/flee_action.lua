local FleeAction = class()

local Point3 = _radiant.csg.Point3

FleeAction.name = 'stonehearth.actions.flee'
FleeAction.does = 'stonehearth.top'
FleeAction.priority = 0

function FleeAction:__init(ai, entity)
   self._entity = entity
   self._ai = ai
   radiant.events.listen('radiant.events.slow_poll', self)
end

FleeAction['radiant.events.slow_poll'] = function(self)
   self._target = radiant.combat.get_target_table_top(self._entity, 'aggro')

   if self._target then
      local priority = -10 * radiant.combat.compare_attribute(self._entity, self._target, 'ferocity')
      self._ai:set_action_priority(self, priority)
   else
      self._ai:set_action_priority(self, 0)
   end
end

--- xxx
function FleeAction:run(ai, entity)
   assert(self._target)
   radiant.log.info('%s is running from %s !!!!!', tostring(self._entity), tostring(self._target))

   local my_location = radiant.entities.get_world_grid_location(self._entity)
   local target_location = radiant.entities.get_world_grid_location(self._target)
   local flee_location = my_location + (my_location - target_location)

   ai:execute('stonehearth.goto_location', flee_location)
end

return FleeAction