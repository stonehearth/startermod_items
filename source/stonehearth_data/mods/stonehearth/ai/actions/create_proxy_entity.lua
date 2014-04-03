local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Entity = _radiant.om.Entity
local CreateProxyEntity = class()

CreateProxyEntity.name = 'create proxy'
CreateProxyEntity.does = 'stonehearth:create_proxy_entity'
CreateProxyEntity.args = {
   location = Point3,     -- location to stick it
}
CreateProxyEntity.think_output = {
   entity = Entity        -- the proxy entity
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
   -- generally speaking, going directly to a location is a strange
   -- thing to do.  why did we not path find to an entity?  why is
   -- this location special?
   
   -- Commentary: No it's not. It's useful when coding ambient behavior
   -- like wandering around, running away, etc. - tom, 9/23/2013

   -- Commenary: Yes it is. =) You don't really know if you can get
   -- to an arbitray location in the world.  For example, if you try to
   -- goto a location inside a box, the pathfinder will just bail with
   -- no solution.  It makes more sense to have a vector based controller
   -- to implement ambient behavior. - tony, 10/01/2013

   -- anyway, the pathfinder can only find paths between two entities,
   -- so go ahead and make a new one.
   
   self:stop()

   self._proxy_entity = radiant.entities.create_entity()
   self._proxy_entity:set_debug_text('go towards location proxy entity')

   -- make the adjacent the same as the entity location, so that we actually walk to the entity location
   local destination = self._proxy_entity:add_component('destination')
   destination:set_region(origin_region)
   destination:set_adjacent(origin_region)

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
