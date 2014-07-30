local Entity = _radiant.om.Entity
local PickupPlacedItemAdjacentTrivial = class()

PickupPlacedItemAdjacentTrivial.name = 'pickup already carried (placed) item'
PickupPlacedItemAdjacentTrivial.does = 'stonehearth:pickup_item'
PickupPlacedItemAdjacentTrivial.args = {
   item = Entity,
}
PickupPlacedItemAdjacentTrivial.version = 2
PickupPlacedItemAdjacentTrivial.priority = 2


local ai = stonehearth.ai
return ai:create_compound_action(PickupPlacedItemAdjacentTrivial)
         :when( function (ai, entity, args)
               return ai.CURRENT.carrying and ai.CURRENT.carrying:get_component('stonehearth:iconic_form') and 
                     ai.CURRENT.carrying:get_component('stonehearth:iconic_form'):get_full_sized_entity() == args.item
            end )
