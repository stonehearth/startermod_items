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

local ai = stonehearth.ai
return ai:create_compound_action(PlaceItem)
         :execute('stonehearth:pickup_item', { item = ai.ARGS.item })
         :execute('stonehearth:drop_carrying_at', { location = ai.ARGS.location })
         :execute('stonehearth:replace_proxy_with_item', { proxy = ai.PREV.item, rotation = ai.ARGS.rotation  })
