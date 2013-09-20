local IdleLookAroundAction = class()

IdleLookAroundAction.name = 'stonehearth.actions.look_around'
IdleLookAroundAction.does = 'stonehearth.activities.idle'
IdleLookAroundAction.priority = 0

function IdleLookAroundAction:__init(ai, entity)
   self._ai = ai;
   self._runInterval = 0
   radiant.events.listen('radiant.events.very_slow_poll', self)
end

IdleLookAroundAction['radiant.events.very_slow_poll'] = function(self)
   if self._runInterval == 0 then
      self._ai:set_action_priority(self, 2)
   end

   self._runInterval = (self._runInterval + 1 ) % 20
end

function IdleLookAroundAction:run(ai, entity)
   ai:execute('stonehearth.activities.run_effect', 'look_around')
   ai:set_action_priority(self, 0)
end

return IdleLookAroundAction