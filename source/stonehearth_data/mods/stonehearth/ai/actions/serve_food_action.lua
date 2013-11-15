--[[
   This class encapsulates running towards a food source and serving
   oneself from it--reaching into it, grabbing a serving item (like a plate or trencher)
   and then going off to eat it. 
]]
radiant.mods.load('stonehearth')
local priorities = require('constants').priorities.needs

local ServeFoodAction = class()

ServeFoodAction.name = 'serve food'
ServeFoodAction.does = 'stonehearth:serve_food'
ServeFoodAction.priority = 5                    --The minute this action is called, it runs

function ServeFoodAction:__init(ai, entity)
   radiant.check.is_entity(entity)
   self._entity = entity                        
   self._ai = ai
 end

--- Run to food, pick up a serving item, and go eat.
function ServeFoodAction:run(ai, entity, path)

   --Am I carrying anything? If so, drop it
   local drop_location = radiant.entities.get_world_grid_location(entity)
   ai:execute('stonehearth:drop_carrying', drop_location)

   -- walk over to the food
   ai:execute('stonehearth:follow_path', path)

   --Put a plate in entity's hand based on the kind of serving dish this is
   --If the food is already gone, abort. If there's no plate entity 
   --specified, just clone the food source
   local food_source = path:get_destination()
   if not food_source then
      ai:abort()
   end

   local plate_type =  radiant.entities.get_entity_data(food_source, 'stonehearth:serving_item')
   if not plate_type then
      ai:abort()
   end
   local plate_entity = radiant.entities.create_entity(plate_type)
   
   radiant.entities.pickup_item(entity, plate_entity)
   ai:execute('stonehearth:run_effect', 'carry_pickup')
   
   --Deduct one serving from the serving entity
   --REVIEW QUESTION: OK TO USE STACKS LIKE THIS?
   if food_source:get_component('item') then
      local item_component = food_source:get_component('item')
      local num_servings = item_component:get_stacks()
      if num_servings > 0 then 
         item_component:set_stacks(num_servings - 1)
         num_servings = num_servings - 1

         -- xxx: localize!!!
         food_source:get_component('unit_info'):set_description('Servings remaining: ' .. num_servings)
      end
      if num_servings <= 0 then
         radiant.entities.destroy_entity(food_source)
      end
   end

   --Run "eat food", which does different things based on hunger levels
   ai:execute('stonehearth:eat_food', plate_entity)
end

return ServeFoodAction
