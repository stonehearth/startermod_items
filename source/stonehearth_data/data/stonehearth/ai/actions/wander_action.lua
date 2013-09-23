local WanderAction = class()

WanderAction.name = 'stonehearth.actions.wander'
WanderAction.does = 'stonehearth.activities.top'
WanderAction.priority = 0

function WanderAction:run(ai, entity)

   local location = radiant.entities.get_location_aligned(entity)
   location.x = location.x + math.random(-3, 3)
   location.z = location.z + math.random(-3, 3)      

   ai:execute('stonehearth.activities.goto_location', location)
   --self._ai:set_action_priority(self, 0)
end

return WanderAction