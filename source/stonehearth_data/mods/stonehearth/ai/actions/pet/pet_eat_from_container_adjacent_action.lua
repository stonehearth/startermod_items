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

   -- consume the a single stack after the effect finishes
   radiant.entities.consume_stack(container, 1)
  
   radiant.entities.turn_to_face(entity, container)
   
   local food = radiant.entities.create_entity(container_data.food)   
   ai:execute('stonehearth:eat_item', { food = food })
end

return PetEatFromContainerAdjacent
