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
   local iconic_component = self._proxy:get_component('stonehearth:iconic_form')
   if iconic_component then
      self._root_entity = iconic_component:get_root_entity()
      assert(self._root_entity and self._root_entity:is_valid())
   end
   ai:set_think_output()
end

function ReplaceProxyWithItem:start(ai, entity, args)
   -- lease the proxy so no one comes out and grabs it from us during our
   -- work animation.
   if not stonehearth.ai:acquire_ai_lease(self._proxy, entity) then
      local lease_component = self._proxy:get_component('stonehearth:lease')
      local owner = 'unknown' 
      if lease_component then
         owner = tostring(lease_component:get_owner('ai_reservation'))
      end
      ai:abort('could not lease %s (%s has it).', tostring(self._proxy), owner)
      return
   end
end

function ReplaceProxyWithItem:run(ai, entity, args)
   if self._root_entity then
      assert(self._root_entity:is_valid())
      ai:execute('stonehearth:run_effect', { effect = 'work' })
      radiant.effects.run_effect(entity, '/stonehearth/data/effects/place_item')

      local location = radiant.entities.get_world_grid_location(self._proxy)
      radiant.terrain.remove_entity(self._proxy)
      
      radiant.entities.turn_to(self._root_entity, args.rotation)
      radiant.terrain.place_entity(self._root_entity, location, { force_iconic = false })
   end
end

function ReplaceProxyWithItem:stop(ai, entity, args)
   if self._proxy and self._proxy:is_valid() then
      -- we got interrupted in a state where the proxy is still alive.  maybe it's sitting
      -- on the ground... (hopefully!)  take the lease off it so someone can do something
      -- with it.
      stonehearth.ai:release_ai_lease(self._proxy, entity)
   end
   self._proxy = nil
end

return ReplaceProxyWithItem
