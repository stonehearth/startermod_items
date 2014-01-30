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
   self._proxy = args.item
   self._proxy_component = item:get_component('stonehearth:placeable_item_proxy')
   if self._proxy_component then
      self._full_sized_entity = self._proxy_component:get_full_sized_entity()
   end
   ai:set_think_output()
end

function ReplaceProxyWithItem:run(ai, entity, args)
   if self._full_sized_entity then
      ai:execute('stonehearth:run_effect', 'work')
      radiant.effects.run_effect(self._entity, { effect = '/stonehearth/data/effects/place_item' })

      radiant.terrain.place_entity(self._full_sized_entity, radiant.entities.get_world_grid_location(self._proxy))
      radiant.entities.turn_to(self._full_sized_entity, rotation)
      radiant.entities.destroy_entity(self._proxy)
   end
end

return ReplaceProxyWithItem
