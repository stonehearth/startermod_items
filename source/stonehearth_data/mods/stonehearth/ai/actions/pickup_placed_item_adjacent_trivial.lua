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
         :when(function (ai, entity, args)
               if not ai.CURRENT.carrying then
                  return false
               end
               local iconic_form = ai.CURRENT.carrying:get_component('stonehearth:iconic_form')
               if not iconic_form then
                  return false
               end
               return iconic_form:get_root_entity() == args.item
            end )
