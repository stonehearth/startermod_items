local PickupItemOnPath = class()

PickupItemOnPath.does = 'stonehearth:pickup_item_on_path'
PickupItemOnPath.priority = 5

function PickupItemOnPath:run(ai, entity, path)
   local item = path:get_destination()
   --TODO: if the destination item changes, we should just abort from inside the pf
   ai:execute('stonehearth:follow_path', path)

   --There's always a chance the item may no longer be there. Fail gracefully
   --TODO: Put up a confused animation
   if not item then
      local name = entity:get_component('unit_info'):get_display_name()
      radiant.log.info('%s (Worker %s): Huh? Where is the thing I was looking for?', tostring(entity), name)
      ai:abort()
   end

   ai:execute('stonehearth:pickup_item_on_ground', item)
end

return PickupItemOnPath
