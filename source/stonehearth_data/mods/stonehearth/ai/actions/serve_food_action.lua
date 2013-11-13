--[[
   Run to a food dish and serve self from it. 
--]]
radiant.mods.load('stonehearth')
local priorities = require('constants').priorities.needs

local ServeFoodAction = class()

ServeFoodAction.name = 'serve food'
ServeFoodAction.does = 'stonehearth:serve_food'
ServeFoodAction.priority = 5                    --The minute this is called, it runs

function ServeFoodAction:__init(ai, entity)
   radiant.check.is_entity(entity)
   self._entity = entity                        --the game character
   self._ai = ai
 end

function ServeFoodAction:run(ai, entity, path)
   --Am I carrying anything? If so, drop it
   local drop_location = radiant.entities.get_world_grid_location(entity)
   ai:execute('stonehearth:drop_carrying', drop_location)

   -- walk over to the food
   ai:execute('stonehearth:follow_path', path)

   --Put a plate in entity's hand based on the kind of serving dish this is
   local food_source = path:get_destination()
   if not food_source then
      --Food source has been destroyed already
      ai:abort()
   end
   local plate_type =  radiant.entities.get_entity_data(food_source, 'stonehearth:serving_item')
   local plate_entity = nil
   --If there's a plate entity, make one. If not, clone the food source 
   if plate_type then
      plate_entity = radiant.entities.create_entity(plate_type)
   else
      plate_entity = radiant.entities.create_entity(food_source:get_uri())
   end
   plate_entity = radiant.entities.create_entity(plate_type)
   radiant.entities.pickup_item(entity, plate_entity)
   ai:execute('stonehearth:run_effect', 'carry_pickup')
   

   --Handle the math on the serving entity
   if food_source:get_component('item') then
      local item_component = food_source:get_component('item')
      local num_servings = item_component:get_stacks()
      if num_servings > 0 then 
         item_component:set_stacks(num_servings - 1)
         num_servings = num_servings - 1
      end
      if num_servings <= 0 then
           radiant.entities.destroy_entity(food_source)
      end
   end

   --Run "eat food", which does different things based on hunger levels
   ai:execute('stonehearth:eat_food', plate_entity)
end

function ServeFoodAction:stop()
end

return ServeFoodAction
