local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Entity = _radiant.om.Entity
local CreateProxyEntity = class()

CreateProxyEntity.name = 'create proxy entity'
CreateProxyEntity.does = 'stonehearth:create_proxy_entity'
CreateProxyEntity.args = {
   location = Point3,
   use_default_adjacent_region = {
      type = 'boolean',
      default = false,
   }
}
CreateProxyEntity.think_output = {
   entity = Entity   -- the proxy entity
}
CreateProxyEntity.version = 2
CreateProxyEntity.priority = 1

function create_origin_region()
   local region = _radiant.sim.alloc_region()
   region:modify(
      function(region3)
         region3:add_unique_cube(
            Cube3(
               Point3(0, 0, 0),
               Point3(1, 1, 1)
            )
         )
      end
   )
   return region
end

local origin_region = create_origin_region()

function CreateProxyEntity:start_thinking(ai, entity, args)
   self:stop()

   self._proxy_entity = radiant.entities.create_entity()
   self._proxy_entity:set_debug_text('go to/toward location proxy entity')

   if not args.use_default_adjacent_region then
      -- make the adjacent the same as the entity location, so that we actually go to the entity location
      local destination = self._proxy_entity:add_component('destination')
      destination:set_region(origin_region)
      destination:set_adjacent(origin_region)
   end

   radiant.terrain.place_entity(self._proxy_entity, args.location)

   ai:set_think_output({ entity = self._proxy_entity })
end

function CreateProxyEntity:stop()
   if self._proxy_entity then
      radiant.entities.destroy_entity(self._proxy_entity)
      self._proxy_entity = nil
   end
end

return CreateProxyEntity
