local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local PlaceItemType = class()

PlaceItemType.name = 'place an item'
PlaceItemType.does = 'stonehearth:place_item_type'
PlaceItemType.args = {
   filter_fn = 'function',
   location = Point3,
   rotation = 'number',
   finish_fn = {
      type = 'function',
      default = function() end,
   }
}
PlaceItemType.version = 2
PlaceItemType.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(PlaceItemType)
         :execute('stonehearth:pickup_item_type', { filter_fn = ai.ARGS.filter_fn })
         :execute('stonehearth:drop_carrying', { location = ai.ARGS.location  })
         :execute('stonehearth:replace_proxy_with_item', { proxy = ai.PREV.item, rotation = ai.ARGS.rotation  })
         :execute('stonehearth:call_function', { fn = ai.ARGS.finish_fn })
