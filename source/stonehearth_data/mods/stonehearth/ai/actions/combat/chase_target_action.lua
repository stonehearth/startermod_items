local ChaseTarget = class()

ChaseTarget.name = 'chase target'
ChaseTarget.does = 'stonehearth.attack.chase_target'
ChaseTarget.priority = 0

function ChaseTarget:run(ai, entity, target)
   if not target then
      target = self._target
   end

   assert(target)
   ai:execute('stonehearth.run_towards_entity', target)
end

return ChaseTarget