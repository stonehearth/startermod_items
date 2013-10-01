local PickupItemOnPath = class()

PickupItemOnPath.does = 'stonehearth.pickup_item_on_path'
PickupItemOnPath.priority = 5

function PickupItemOnPath:run(ai, entity, path)
   local item = path:get_destination()
   ai:execute('stonehearth.follow_path', path)

   --There's always a chance the item may no longer be there. Fail gracefully
   --TODO: Put up a confused animation
   if not item then
      local name = entity:get_component('unit_info'):get_display_name()
      local item_name = item:get_component('unit_info'):get_display_name()
      radiant.log.info('Worker %s: Huh? Where is %s?', name, item_name)
      ai:abort()
   end

   ai:execute('stonehearth.pickup_item', item)
end

return PickupItemOnPath
