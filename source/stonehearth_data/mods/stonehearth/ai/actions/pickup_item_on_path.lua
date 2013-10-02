local PickupItemOnPath = class()

PickupItemOnPath.does = 'stonehearth.pickup_item_on_path'
PickupItemOnPath.priority = 5

function PickupItemOnPath:run(ai, entity, path)
   local item = path:get_destination()
   -- verify it's on the terrain...
   ai:execute('stonehearth.follow_path', path)
   ai:execute('stonehearth.pickup_item_on_ground', item)
end

return PickupItemOnPath
