local event_service = stonehearth.events

local EatCarrying = class()
EatCarrying.name = 'eat carrying'
EatCarrying.does = 'stonehearth:eat_carrying'
EatCarrying.args = { }
EatCarrying.version = 2
EatCarrying.priority = 1

--- Eat the food we're holding in our hands
--  If we're very hungry, bolt it down nearby, even if we've got a path to a chair.
--  As a result, we'll get less full from the food.  
--  If we're moderately hungry, and have a chair, go there and eat the food. This 
--  grants a bonus to how full we feel after eating. 
--  If we're moderately hungry and don't have a chair, find a random location at
--  moderate distance and eat leisurely. This has no effect on satiation. 
function EatCarrying:run(ai, entity)
   local food = radiant.entities.get_carrying(entity)
   if not food then
      ai:abort('cannot eat.  not carrying anything!')
   end

   self:_get_eat_data()

   local times = self._eat_data.effect_loops and self._eat_data.effect_loops or 3
   for i = 1, times do
      ai:execute('stonehearth:run_effect', 'eat')
   end
   local attributes_component = entity:add_component('stonehearth:attributes')
   local hunger = attributes_component:get_attribute('hunger')
   entity_attribute_comp:set_attribute('hunger', hunger - eat_data.satisfaction)

   if self._eat_data.journal_message then
      radiant.events.trigger(personality_service, 'stonehearth:journal_event', 
                             {entity = entity, description = eat_data.journal_message})
   end
end

function EatCarrying:_get_eat_data()
   local posture = radiant.entities.get_posture(entity)
   local food_data = radiant.entities.get_entity_data(food, 'stonehearth:food')
   if food_data then
      self._eat_data = food_data[posture]
      if not eat_data then
         self._eat_data = food_data.default
      end
   end
   if not self._eat_data then
      ai:abort('no eat data for posture "%s" and no default!', tostring(posture))
   end
end

return EatCarrying
