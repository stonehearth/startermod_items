local Entity = _radiant.om.Entity

-- xxx: omg, factor the container bits out of this action (better decomp and
-- extensibility!)
local EatItem = class()
EatItem.name = 'eat item'
EatItem.does = 'stonehearth:eat_item'
EatItem.args = {
   food = Entity,
}
EatItem.version = 2
EatItem.priority = 1

function EatItem:run(ai, entity, args)
   local food = args.food

   self._food_data = self:_get_food_data(food, entity)
   if not self._food_data then
      ai:abort('Cannot eat: No food data for %s.', tostring(food))
   end

   ai:execute('stonehearth:run_effect', {
      effect = 'eat',
      times = self._food_data.effect_loops or 3
   })

   -- journal stuff will get refactored at some point
   if self._food_data.journal_message then
      radiant.events.trigger_async(stonehearth.personality, 'stonehearth:journal_event', 
                                   {entity = entity, description = self._food_data.journal_message})
   end
end

function EatItem:stop(ai, entity, args)
   ai:set_status_text('eating ' .. radiant.entities.get_name(args.food))
end   

function EatItem:stop(ai, entity, args)
   local attributes_component = entity:add_component('stonehearth:attributes')

   if self._food_data then
      -- if we're interrupted, go ahead and immediately finish eating
      -- we decided it was not fun to leave this hanging
      local satisfaction = self._food_data.satisfaction
      local calories = attributes_component:get_attribute('calories')
      local new_calories_value = calories + satisfaction
      if new_calories_value >= stonehearth.constants.food.MAX_ENERGY then
         new_calories_value = stonehearth.constants.food.MAX_ENERGY 
      end
      radiant.entities.destroy_entity(args.food)

      -- finally, adjust calories if necessary.  this might trigger callbacks which
      -- result in destroying the action, so make sure we do it LAST! (see calorie_obserer.lua)
      if new_calories_value then
         attributes_component:set_attribute('calories', new_calories_value)
      end

      self._food_data = nil
   end
end

function EatItem:_get_food_data(food, entity)
   local food_entity_data = radiant.entities.get_entity_data(food, 'stonehearth:food')
   local food_data

   if food_entity_data then
      local posture = radiant.entities.get_posture(entity)
      food_data = food_entity_data[posture]

      if not food_data then
         food_data = food_entity_data.default
      end
   end
   
   return food_data
end

return EatItem
