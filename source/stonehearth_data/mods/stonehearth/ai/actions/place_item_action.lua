local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local PlaceItem = class()

PlaceItem.name = 'place an item'
PlaceItem.does = 'stonehearth:place_item'
PlaceItem.args = {
   item = Entity,       -- the item to place.  must have a 'stonehearth:entity_forms' component!
   location = Point3,   -- where to take it.  the ghost should already be there
   rotation = 'number'  -- the rotation
}
PlaceItem.version = 2
PlaceItem.priority = 1

function PlaceItem:start_thinking(ai, entity, args)
   -- xxx: we should not need this when guard.  the task framework should be intelligent enough to validate
   -- the arguments to the task before attempting to start the actions.  in this case, the item we want to 
   -- place get destroyed before we could get around to doing it, either because someone tried to put the
   -- item in two different locations or because they used the generic place item interface *and* the specific
   -- interface, but the generic place item got to it first.  either way, don't try to run this action if the
   -- item that needs to be place no longer exists!
   if args.item:is_valid() then
      ai:set_think_output()
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(PlaceItem)
         :execute('stonehearth:pickup_item', { item = ai.ARGS.item })
         :execute('stonehearth:drop_carrying_at', { location = ai.ARGS.location })
         :execute('stonehearth:replace_proxy_with_item', { proxy = ai.PREV.item, rotation = ai.ARGS.rotation  })
