local ChaseAction = class()

ChaseAction.name = 'stonehearth.actions.chase'
ChaseAction.does = 'stonehearth.activities.top'
ChaseAction.priority = 0

function ChaseAction:__init(ai, entity)
   self._entity = entity
   self._ai = ai
end

function ChaseAction:run(ai, entity)

end


return ChaseAction