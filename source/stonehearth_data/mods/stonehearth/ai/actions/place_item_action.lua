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
         :execute('stonehearth:pickup_item', { item = ai.ARGS.item })
         :execute('stonehearth:drop_carrying', { location = ai.ARGS.location  })
         :execute('stonehearth:replace_proxy_with_item', { proxy = ai.PREV.item, rotation = ai.ARGS.rotation  })
         :execute('stonehearth:call_function', { fn = ai.ARGS.finish_fn })
