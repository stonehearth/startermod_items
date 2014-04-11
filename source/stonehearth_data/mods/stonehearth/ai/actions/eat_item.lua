local Entity = _radiant.om.Entity

-- xxx: omg, factor the container bits out of this action (better decomp and
-- extensibility!)
local EatItem = class()
EatItem.name = 'eat item'
EatItem.does = 'stonehearth:eat_item'
EatItem.args = {
   item = Entity,
   face_item = {
      type = 'boolean',
      default = true
   }
}
EatItem.version = 2
EatItem.priority = 1

function EatItem:run(ai, entity, args)
   local item = args.item
   local facing_entity = nil

   if args.face_item then
      facing_entity = item
   end

   self._container_data = radiant.entities.get_entity_data(item, 'stonehearth:food_container')
   if self._container_data then
      self._food = radiant.entities.create_entity(self._container_data.food)
   else
      self._food = item
   end

   self._food_data = self:_get_food_data(self._food, entity)

   if not self._food_data then
      ai:abort('Cannot eat: No food data for %s.', tostring(self._food))
   end

   ai:execute('stonehearth:run_effect', {
      effect = 'eat',
      times = self._food_data.effect_loops or 3,
      facing_entity = facing_entity
   })

   -- journal stuff will get refactored at some point
   if self._food_data.journal_message then
      radiant.events.trigger_async(stonehearth.personality, 'stonehearth:journal_event', 
                                   {entity = entity, description = self._food_data.journal_message})
   end
end

function EatItem:stop(ai, entity, args)
   local consumed
   local new_calories_value
   local attributes_component = entity:add_component('stonehearth:attributes')

   -- if we're interrupted, go ahead and immediately finish eating
   -- we decided it was not fun to leave this hanging
   if self._food then
      if self._container_data then
         local container = args.item
         consumed = radiant.entities.consume_stack(container)
         if not consumed then
            ai:get_log():info('%s cannot eat: Food container is now empty.', tostring(entity))
         end
         self._container_data = nil
      else
         consumed = true
      end

      if consumed then
         if self._food_data then
            local satisfaction = self._food_data.satisfaction
            local calories = attributes_component:get_attribute('calories')
            new_calories_value = calories + satisfaction
            if new_calories_value >= stonehearth.constants.food.MAX_ENERGY then
               new_calories_value = stonehearth.constants.food.MAX_ENERGY 
            end
            self._food_data = nil
         end
      end

      radiant.entities.destroy_entity(self._food)
      self._food = nil

      -- finally, adjust calories if necessary.  this might trigger callbacks which
      -- result in destroying the action, so make sure we do it LAST! (see calorie_obserer.lua)
      if new_calories_value then
         attributes_component:set_attribute('calories', new_calories_value)
      end
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
