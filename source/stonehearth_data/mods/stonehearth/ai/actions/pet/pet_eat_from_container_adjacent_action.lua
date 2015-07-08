local Entity = _radiant.om.Entity

-- xxx: omg, factor the container bits out of this action (better decomp and
-- extensibility!)
local PetEatFromContainerAdjacent = class()
PetEatFromContainerAdjacent.name = 'eat item'
PetEatFromContainerAdjacent.does = 'stonehearth:pet_eat_from_container_adjacent'
PetEatFromContainerAdjacent.args = {
   container = Entity
}
PetEatFromContainerAdjacent.version = 2
PetEatFromContainerAdjacent.priority = 1

function PetEatFromContainerAdjacent:run(ai, entity, args)
   -- sadly, copied from

   local container = args.container
   local container_data = radiant.entities.get_entity_data(container, 'stonehearth:food_container')
   if not container_data then
      ai:abort("%s has no stonehearth:food_container entity data", tostring(container))
      return
   end
   
   radiant.entities.turn_to_face(entity, container)   
   local food = radiant.entities.create_entity(container_data.food)
   ai:execute('stonehearth:eat_item', { food = food })
   
   ai:unprotect_argument(container)
   -- consume the a single stack after the eat finishes
   if not radiant.entities.consume_stack(container, 1) then
      ai:abort('Cannot eat: Food container is empty.')
   end
end

return PetEatFromContainerAdjacent
