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
   local region = Region3(
      Cube3(
         Point3(0, 0, 0),
         Point3(size.x, 1, size.z)
      )
   )

   self:_set_region_component(entity, 'region_collision_shape', region)
      :set_region_collision_type(_radiant.om.RegionCollisionShape.NONE)

   self:_set_region_component(entity, 'destination', region)
      :set_auto_update_adjacent(true)
end

function TrappingService:_set_region_component(entity, component_name, region)
   local region_boxed = _radiant.sim.alloc_region3()

   region_boxed:modify(
      function (region3)
         region3:add_region(region)
      end
   )

   local component = entity:add_component(component_name)
   component:set_region(region_boxed)

   return component
end

return TrappingService
