local IdleLookAroundAction = class()

IdleLookAroundAction.name = 'stonehearth.actions.look_around'
IdleLookAroundAction.does = 'stonehearth.idle'
IdleLookAroundAction.priority = 0

function IdleLookAroundAction:__init(ai, entity)
   self._ai = ai;
   self:set_next_run();
   radiant.events.listen('radiant.events.very_slow_poll', self)
end

IdleLookAroundAction['radiant.events.very_slow_poll'] = function(self)
   if self._runInterval == 0 then
      self._ai:set_action_priority(self, 2)
      self:set_next_run()
   end

   self._runInterval = self._runInterval - 1
end

function IdleLookAroundAction:run(ai, entity)
   ai:execute('stonehearth.run_effect', 'look_around')
   ai:set_action_priority(self, 0)
end

function IdleLookAroundAction:set_next_run(ai, entity)
   self._runInterval = math.random(15, 20)
end

return IdleLookAroundAction