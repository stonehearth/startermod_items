local Point3 = _radiant.csg.Point3

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

   --If the item has a placed_item_component, it's too big to pick up normally
   --and it might be in use. If it's in use, do not pick it up! Just abort.
   --If it's not in use, repalce it with its proxy and continue
   local placed_item_component = item:get_component('stonehearth:placed_item')
   if placed_item_component then
      local item_location = item:get_component('mob'):get_world_grid_location()

      local little_item = radiant.entities.create_entity(placed_item_component:get_proxy())
      ai:execute('stonehearth:run_effect', 'work')
      radiant.entities.destroy_entity(item)

      --TODO: instead of destroy, add to the proxy and revive later
      --TODO: debug why this doesn't work; adding/removing the same entity from terrain
      --adds back an invisible version of the entity (see placement test)
      --local proxy_component = little_item:add_component('stonehearth:placeable_item_proxy')
      --proxy_component:set_full_sized_entity(item)
      --radiant.terrain.remove_entity(item)

      radiant.terrain.place_entity(little_item, item_location)
      item = little_item
   end

   ai:execute('stonehearth:pickup_item_on_ground', item)
end

return PickupItemOnPath
