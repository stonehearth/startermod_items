local Point3 = _radiant.csg.Point3

local PickupItemOnPath = class()

PickupItemOnPath.name = 'pickup item on path'
PickupItemOnPath.does = 'stonehearth:pickup_item_on_path'
PickupItemOnPath.priority = 5

function PickupItemOnPath:run(ai, entity, path)
   local item = path:get_destination()
   self._item_trace = radiant.entities.trace_location(item, 'pickup item on path')
      :on_changed(function()
         ai:abort('path destination changed')
      end)
      :on_destroyed(function()
         ai:abort('path destination destroyed')
      end)

   ai:execute('stonehearth:follow_path', path)

   --If the item has a placed_item_component, it's too big to pick up normally
   --and it might be in use. If it's in use, do not pick it up! Just abort.
   --If it's not in use, repalce it with its proxy and continue
   local placed_item_component = item:get_component('stonehearth:placed_item')
   if placed_item_component then
      local item_location = item:get_component('mob'):get_world_grid_location()
      local little_item = radiant.entities.create_entity(placed_item_component:get_proxy())
      ai:execute('stonehearth:run_effect', 'work')
      
      self:_clear_item_trace()
      local proxy_component = little_item:add_component('stonehearth:placeable_item_proxy')
      proxy_component:set_full_sized_entity(item)
      radiant.terrain.remove_entity(item)
      radiant.terrain.place_entity(little_item, item_location)
      item = little_item
   end
   self:_clear_item_trace()
   ai:execute('stonehearth:pickup_item_on_ground', item)
end

function PickupItemOnPath:stop()
   self:_clear_item_trace()
end

function PickupItemOnPath:_clear_item_trace()
   if self._item_trace then
      self._item_trace:destroy()
      self._item_trace = nil
   end
end

return PickupItemOnPath
