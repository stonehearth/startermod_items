local Entity = _radiant.om.Entity
local PickupPlacedItemAdjacentTrivial = class()

PickupPlacedItemAdjacentTrivial.name = 'pickup already carried (placed) item'
PickupPlacedItemAdjacentTrivial.does = 'stonehearth:pickup_item'
PickupPlacedItemAdjacentTrivial.args = {
   item = Entity,
}
PickupPlacedItemAdjacentTrivial.version = 2
PickupPlacedItemAdjacentTrivial.priority = 2

function PickupPlacedItemAdjacentTrivial:start_thinking(ai, entity, args)
   local item = ai.CURRENT.carrying

   if item then
      local iconic_form = item:get_component('stonehearth:iconic_form')
      if iconic_form and iconic_form:get_root_entity() == args.item then
         ai:set_think_output()
      end
   end
end

return PickupPlacedItemAdjacentTrivial
