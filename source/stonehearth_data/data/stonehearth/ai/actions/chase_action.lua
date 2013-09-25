local ChaseAction = class()

ChaseAction.name = 'stonehearth.actions.chase'
ChaseAction.does = 'stonehearth.activities.top'
ChaseAction.priority = 0

function ChaseAction:__init(ai, entity)
   self._entity = entity
   self._ai = ai
   radiant.events.listen('radiant.events.very_slow_poll', self)
end

ChaseAction['radiant.events.very_slow_poll'] = function(self)
   local items = {}
   local sensor 
   local sensor_list = self._entity:get_component('sensor_list')

   if sensor_list then
      sensor = sensor_list:get_sensor('sight')
      local contents = sensor:get_contents()
      local seen = contents:items() 
      -- for id in sensor:get_contents():items() do table.insert(items, id) end
      -- radiant.log.info(seen)
   end
end

function ChaseAction:run(ai, entity)

end


return ChaseAction