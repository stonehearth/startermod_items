local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local ReplaceProxyWithItem = class()

ReplaceProxyWithItem.name = 'replace proxy with item'
ReplaceProxyWithItem.does = 'stonehearth:replace_proxy_with_item'
ReplaceProxyWithItem.args = {
   proxy = Entity,         -- the proxy
   rotation = 'number',    -- rotation to apply to the item
}
ReplaceProxyWithItem.version = 2
ReplaceProxyWithItem.priority = 2

function ReplaceProxyWithItem:start_thinking(ai, entity, args)   
   self._proxy = args.proxy
   self._proxy_component = self._proxy:get_component('stonehearth:placeable_item_proxy')
   if self._proxy_component then
      self._full_sized_entity = self._proxy_component:get_full_sized_entity()
      assert(self._full_sized_entity and self._full_sized_entity:is_valid())
   end
   ai:set_think_output()
end

function ReplaceProxyWithItem:start(ai, entity, args)
   -- lease the proxy so no one comes out and grabs it from us during our
   -- work animation.
   local lease_component = self._proxy:add_component('stonehearth:lease')
   if not lease_component:acquire('ai_reservation', entity) then
      ai:abort('could not lease %s (%s has it).', tostring(self._proxy), tostring(lease_component:get_owner('ai_reservation')))
      return
   end
end

function ReplaceProxyWithItem:run(ai, entity, args)
   if self._full_sized_entity then
      assert(self._full_sized_entity:is_valid())
      ai:execute('stonehearth:run_effect', { effect = 'work' })
      radiant.effects.run_effect(entity, '/stonehearth/data/effects/place_item')

      radiant.terrain.place_entity(self._full_sized_entity, radiant.entities.get_world_grid_location(self._proxy))
      radiant.entities.turn_to(self._full_sized_entity, args.rotation)
      radiant.entities.destroy_entity(self._proxy)
      self._proxy = nil
   end
end

function ReplaceProxyWithItem:stop(ai, entity, args)
   if self._proxy and self._proxy:is_valid() then
      -- we got interrupted in a state where the proxy is still alive.  maybe it's sitting
      -- on the ground... (hopefully!)  take the lease off it so someone can do something
      -- with it.
      local lease_component = self._proxy:add_component('stonehearth:lease')
      if lease_component then
         lease_component:release('ai_reservation', entity)
      end
      self._proxy = nil
   end
end

return ReplaceProxyWithItem
