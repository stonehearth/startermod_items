local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

MiningService = class()

local XZ_ALIGN = 4
local Y_ALIGN = 5

function MiningService:initialize()
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.initialized = true
   else
   end
end

function MiningService:destroy()
end

function MiningService:create_mining_zone(player_id, faction)
   local mining_zone = radiant.entities.create_entity('stonehearth:mining_zone')

   mining_zone:add_component('unit_info')
      :set_player_id(player_id)
      :set_faction(faction)

   return mining_zone
end

function MiningService:dig_down(mining_zone, region)
   self:_dig_impl(mining_zone, region, function(cube)
         return self:_get_aligned_cube(cube)
      end)
end

function MiningService:dig_out(mining_zone, region)
   self:_dig_impl(mining_zone, region, function(cube)
         local aligned_cube = self:_get_aligned_cube(cube)
         local min, max = aligned_cube.min, aligned_cube.max

         -- drop the max y level by 1 to preserve the ceiling
         return Cube3(
               min,
               Point3(max.x, max.y-1, max.z)
            )
      end)
end

function MiningService:dig_up(mining_zone, region)
   self:_dig_impl(mining_zone, region, function(cube)
         local aligned_cube = self:_get_aligned_cube(cube)
         local min, max = aligned_cube.min, aligned_cube.max

         -- ok this region API definition is weird
         return Cube3(
               Point3(min.x, min.y-1, min.z),
               Point3(max.x, max.y-1, max.z)
            )
      end)
end

function MiningService:_dig_impl(mining_zone, region, cube_transform)
   local transformed_region = Region3()
   
   for cube in region:each_cube() do
      local transformed_cube = cube_transform(cube)
      transformed_region:add_cube(transformed_cube)
   end

   self:_add_region_to_zone(mining_zone, transformed_region)
end

function MiningService:_add_region_to_zone(mining_zone, region)
   if not region or region:empty() then
      return
   end

   local mining_zone_component = mining_zone:add_component('stonehearth:mining_zone')
   local boxed_region = mining_zone_component:get_region()
   local location

   if boxed_region:get():empty() then
      location = region:get_bounds().min
      radiant.terrain.place_entity_at_exact_location(mining_zone, location)
   else
      location = radiant.entities.get_world_grid_location(mining_zone)
   end

   boxed_region:modify(function(cursor)
         -- could avoid a region copy if we're willing to modify the input region
         local local_region = region:translated(-location)
         cursor:add_region(local_region)
      end)
end

function MiningService:_get_aligned_cube(cube)
   local min, max = cube.min, cube.max
   local aligned_min = Point3(
         math.floor(min.x / XZ_ALIGN) * XZ_ALIGN,
         math.floor(min.y / Y_ALIGN) * Y_ALIGN,
         math.floor(min.z / XZ_ALIGN) * XZ_ALIGN
      )
   local aligned_max = Point3(
         math.ceil(max.x / XZ_ALIGN) * XZ_ALIGN,
         math.ceil(max.y / Y_ALIGN) * Y_ALIGN,
         math.ceil(max.z / XZ_ALIGN) * XZ_ALIGN
      )
   return Cube3(aligned_min, aligned_max)
end

return MiningService
