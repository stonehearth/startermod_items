local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local rng = _radiant.csg.get_default_rng()

TrappingService = class()

function TrappingService:initialize()
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.initialized = true
   else
   end
end

function TrappingService:destroy()
end

function TrappingService:designate_trapping_grounds(player_id, faction, location, size)
   local entity = radiant.entities.create_entity('stonehearth:trapper:trapping_grounds')

   entity:get_component('unit_info'):set_player_id(player_id)
   entity:get_component('unit_info'):set_faction(faction)

   self:_add_region_components(entity, size)

   local trapping_grounds_component = entity:add_component('stonehearth:trapping_grounds')
   trapping_grounds_component:set_size(size.x, size.z)

   radiant.terrain.place_entity(entity, location)

   self.__saved_variables:mark_changed()

   trapping_grounds_component:start_tasks()

   return entity
end

function TrappingService:_add_region_components(entity, size)
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

return TrappingService
