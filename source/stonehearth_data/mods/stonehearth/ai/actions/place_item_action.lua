local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local PlaceItem = class()

PlaceItem.name = 'place an item'
PlaceItem.does = 'stonehearth:place_item'
PlaceItem.args = {
   item = Entity,
   location = Point3,
   rotation = 'number',
   finish_fn = {
      type = 'function',
      default = function() end,
   }
}
PlaceItem.version = 2
PlaceItem.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(PlaceItem)
         -- xxx: we should not need this when guard.  the take framework should be intelligent enough to validate
         -- the arguments to the task before attempting to start the actions.  in this case, the item we want to 
         -- place get destroyed before we could get around to doing it, either because someone tried to put the
         -- item in two different locations or because they used the generic place item interface *and* the specific
         -- interface, but the generic place item got to it first.  either way, don't try to run this action if the
         -- item that needs to be place no longer exists!
         :when(function () return ai.ARGS.item and ai.ARGS.item:is_valid() end)
         :execute('stonehearth:pickup_item', { item = ai.ARGS.item })
         :execute('stonehearth:drop_carrying', { location = ai.ARGS.location  })
         :execute('stonehearth:replace_proxy_with_item', { proxy = ai.PREV.item, rotation = ai.ARGS.rotation  })
         :execute('stonehearth:call_function', { fn = ai.ARGS.finish_fn })
