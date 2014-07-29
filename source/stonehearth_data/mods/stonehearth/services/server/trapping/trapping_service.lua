local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

TrappingService = class()

function TrappingService:__init()
end

function TrappingService:initialize()
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
   else
   end
end

function TrappingService:destroy()
end

function TrappingService:designate_trapping_grounds(player_id, faction, location, size)
   local entity = radiant.entities.create_entity('stonehearth:trapper:trapping_grounds')
   radiant.terrain.place_entity(entity, location)
   entity:add_component('stonehearth:trapping_grounds'):set_size(size)

   self:_add_collision_region(entity, size)

   entity:get_component('unit_info'):set_player_id(player_id)
   entity:get_component('unit_info'):set_faction(faction)

   self.__saved_variables:mark_changed()
   return entity
end

function TrappingService:_add_collision_region(entity, size)
   local collision_component = entity:add_component('region_collision_shape')
   local collision_region_boxed = _radiant.sim.alloc_region()

   collision_region_boxed:modify(
      function (region3)
         region3:add_unique_cube(
            Cube3(
               -- recall that region_collision_shape is in local coordiantes
               Point3(0, 0, 0),
               Point3(size.x, 1, size.y)
            )
         )
      end
   )

   collision_component:set_region(collision_region_boxed)
   collision_component:set_region_collision_type(_radiant.om.RegionCollisionShape.NONE)
end

return TrappingService
