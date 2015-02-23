local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

ShepherdService = class()

function ShepherdService:initialize()
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.initialized = true
   else
   end

end

function ShepherdService:create_new_pasture(session, location, size)
   -- A little sanitization: what we get from the client isn't exactly a Point3
   location = Point3(location.x, location.y, location.z)
   local entity = radiant.entities.create_entity('stonehearth:shepherd:shepherd_pasture', { owner = session.player_id })
   entity:get_component('unit_info')
            :set_player_id(session.player_id)
   self:_add_region_components(entity, size)

   local pasture_component = entity:get_component('stonehearth:shepherd_pasture')
   pasture_component:set_size(size.x, size.z)
   pasture_component:set_pasture_type()

   radiant.terrain.place_entity(entity, location)

   return entity

end

function ShepherdService:_add_region_components(entity, size)
   local shape = Cube3(Point3.zero, Point3(size.x, 1, size.z))

   entity:add_component('region_collision_shape')
            :set_region_collision_type(_radiant.om.RegionCollisionShape.NONE)
            :set_region(_radiant.sim.alloc_region3())
            :get_region():modify(function(cursor)
                  cursor:add_unique_cube(shape)
               end)

   entity:add_component('destination')
            :set_auto_update_adjacent(true)
            :set_region(_radiant.sim.alloc_region3())
            :get_region():modify(function(cursor)
                  cursor:add_unique_cube(shape)
               end)
end

return ShepherdService