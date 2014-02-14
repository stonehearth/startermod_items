local CompelledBehaviorAction = class()

CompelledBehaviorAction.name = 'struggle'
CompelledBehaviorAction.does = 'stonehearth:top'
CompelledBehaviorAction.args = {}
CompelledBehaviorAction.version = 2
CompelledBehaviorAction.priority = stonehearth.constants.priorities.top.STUCK

function CompelledBehaviorAction:run(ai, entity)
   -- todo, insert a saving throw
   while true
      ai:execute('stonehearth:run_effect', { effect = 'idle_look_around'})
   end
end

return CompelledBehaviorAction
