local Entity = _radiant.om.Entity

local GetFoodFromContainerAdjacent = class()
GetFoodFromContainerAdjacent.name = 'get food from container adjacent'
GetFoodFromContainerAdjacent.does = 'stonehearth:get_food_from_container_adjacent'
GetFoodFromContainerAdjacent.args = {
   container = Entity      -- the container with the food in it
}
GetFoodFromContainerAdjacent.version = 2
GetFoodFromContainerAdjacent.priority = 1

--- Run to food, pick up a serving item, and go eat.
function GetFoodFromContainerAdjacent:run(ai, entity, args)
   local container = args.container

   local container_data = radiant.entities.get_entity_data(container, 'stonehearth:food_container')
   if not container_data then
      ai:abort("%s has no stonehearth:food_container entity data", tostring(container))
      return
   end

   local food = radiant.entities.create_entity(container_data.food)
   
   radiant.entities.turn_to_face(entity, container)
   ai:execute('stonehearth:run_effect', { effect = container_data.effect })


   -- consume the stack after the effect finishes
   local stacks_per_serving = container_data.stacks_per_serving or 1
   if not radiant.entities.consume_stack(container, stacks_per_serving) then
      radiant.entities.destroy_entity(food)
      ai:abort('Cannot eat: Food container is empty.')
   end

   radiant.entities.pickup_item(entity, food)
end

return GetFoodFromContainerAdjacent
