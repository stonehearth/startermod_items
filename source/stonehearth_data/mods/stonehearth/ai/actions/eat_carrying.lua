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
   self._food = radiant.entities.get_carrying(entity)
   if not self._food then
      ai:abort('cannot eat.  not carrying anything!')
   end

   local food_data = self:_get_food_data(ai, entity, self._food)

   local times = food_data.effect_loops and food_data.effect_loops or 3
   for i = 1, times do
      ai:execute('stonehearth:run_effect', { effect = 'eat' })
   end
   local attributes_component = entity:add_component('stonehearth:attributes')
   local hunger = attributes_component:get_attribute('hunger')
   attributes_component:set_attribute('hunger', hunger - food_data.satisfaction)

   if food_data.journal_message then
      radiant.events.trigger(stonehearth.personality, 'stonehearth:journal_event', 
                             {entity = entity, description = food_data.journal_message})
   end
end

function EatCarrying:stop(ai, entity, args)
   if self._food then
      radiant.entities.remove_carrying(entity)
      radiant.entities.destroy_entity(self._food)
      self._food = nil
   end
end

function EatCarrying:_get_food_data(ai, entity, food)
   local food_data
   local food_entity_data = radiant.entities.get_entity_data(food, 'stonehearth:food')
   if food_entity_data then
      local posture = radiant.entities.get_posture(entity)
      food_data = food_entity_data[posture]
      if not food_data then
         food_data = food_entity_data.default
      end
   end
   if not food_data then
      ai:abort('no eat data for posture "%s" and no default!', tostring(posture))
   end
   return food_data
end

return EatCarrying
