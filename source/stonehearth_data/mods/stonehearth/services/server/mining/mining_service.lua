local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('mining')

MiningService = class()

local XZ_ALIGN = 4
local Y_ALIGN = 5
local MAX_REACH_UP = 3
local MAX_REACH_DOWN = 1

function MiningService:initialize()
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.initialized = true
   else
   end
end

function MiningService:destroy()
end

----------------------------------------------------------------------------
-- Use the dig_* methods to let the mining service manage the zones for you.
-- Use the create_mining_zone, add_region_to_zone, and merge_zones methods 
-- to explicitly manage the zones yourself.
----------------------------------------------------------------------------

-- dig out an arbitary region
function MiningService:dig_region(player_id, faction, region)
   local inflated_region = region:inflated(Point3.one)

   -- using town as a proxy for the eventual player object
   local town = stonehearth.town:get_town(player_id)
   local mining_zones = town:get_mining_zones()
   local mergable_zones = {}

   -- find all the existing mining zones that are adjacent or overlap the new region
   for _, zone in pairs(mining_zones) do
      -- move the inflated region to the local space of the zone
      local location = radiant.entities.get_world_grid_location(zone)
      local translated_inflated_region = inflated_region:translated(-location)

      local mining_zone_component = zone:add_component('stonehearth:mining_zone')
      local existing_region = mining_zone_component:get_region():get()

      if existing_region:intersects(translated_inflated_region) then
         mergable_zones[zone:get_id()] = zone
      end
   end

   local _, selected_zone = next(mergable_zones)

   if selected_zone then
      -- remove the surviving zone from the merge list
      mergable_zones[selected_zone:get_id()] = nil
   else
      -- no adjacent or overlapping zone exists, so create a new one
      selected_zone = self:create_mining_zone(player_id, faction)
      town:add_mining_zone(selected_zone)
   end

   self:add_region_to_zone(selected_zone, region)

   -- merge the other zones into the surviving zone and destroy them
   for _, zone in pairs(mergable_zones) do
      self:merge_zones(selected_zone, zone)
   end

   return selected_zone
end

-- dig down, quantized to the 4x5x4 mining cells
function MiningService:dig_down(player_id, faction, region)
   local aligned_region = self:_transform_cubes_in_region(region, function(cube)
         return self:_get_aligned_cube(cube)
      end)
   self:dig_region(player_id, faction, aligned_region)
end

-- dig out, quantized to the 4x5x4 cells, but preserve the ceiling
function MiningService:dig_out(player_id, faction, region)
   local aligned_region = self:_transform_cubes_in_region(region, function(cube)
         local aligned_cube = self:_get_aligned_cube(cube)
         local min, max = aligned_cube.min, aligned_cube.max

         -- drop the max y level by 1 to preserve the ceiling
         return Cube3(
               min,
               Point3(max.x, max.y-1, max.z)
            )
      end)
   self:dig_region(player_id, faction, aligned_region)
end

-- dig up, quantized to the 4x5x4 cells
function MiningService:dig_up(player_id, faction, region)
   local aligned_region = self:_transform_cubes_in_region(region, function(cube)
         local aligned_cube = self:_get_aligned_cube(cube)
         local min, max = aligned_cube.min, aligned_cube.max

         -- ok this region API definition is weird
         return Cube3(
               Point3(min.x, min.y-1, min.z),
               Point3(max.x, max.y-1, max.z)
            )
      end)
   self:dig_region(player_id, faction, aligned_region)
end

-- explicitly create a mining zone
function MiningService:create_mining_zone(player_id, faction)
   local mining_zone = radiant.entities.create_entity('stonehearth:mining_zone')

   mining_zone:add_component('unit_info')
      :set_player_id(player_id)
      :set_faction(faction)

   return mining_zone
end

-- explicitly add a region to a mining zone
function MiningService:add_region_to_zone(mining_zone, region)
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

-- merges zone2 into zone1, and destroys zone2
function MiningService:merge_zones(zone1, zone2)
   local boxed_region1 = zone1:add_component('stonehearth:mining_zone'):get_region()
   local boxed_region2 = zone2:add_component('stonehearth:mining_zone'):get_region()
   local location1 = radiant.entities.get_world_grid_location(zone1)
   local location2 = radiant.entities.get_world_grid_location(zone2)

   boxed_region1:modify(function(region1)
         -- move region2 into the local space of region1
         local translation = location2 - location1
         local region2 = boxed_region2:get():translated(translation)
         region1:add_region(region2)
      end)

   radiant.entities.destroy_entity(zone2)
end

function MiningService:resolve_point_of_interest(from, mining_zone, default_poi)
   local location = radiant.entities.get_world_grid_location(mining_zone)
   local destination_component = mining_zone:add_component('destination')
   local destination_region = destination_component:get_region():get()

   -- get the reachable region in local coordinates to the zone
   local reachable_region = self:get_reachable_region(from - location)
   local reachable_destination = reachable_region:intersected(destination_region)
   local max

   if not reachable_destination:empty() then
      -- pick any highest point in the region
      for cube in reachable_destination:each_cube() do
         if not max or cube.max.y > max.y then
            max = cube.max
         end
      end

      -- strip one off the max to get terrain coordinates of block
      -- then convert to world coordinates
      local poi = max - Point3.one + location
      assert(poi ~= from - Point3.unit_y)
      return poi
   else
      log:error('not adjacent to a minable point')   
      return default_poi
   end
end

-- return all the locations that can be reached from point
function MiningService:get_reachable_region(location)
   local y_min = location.y - MAX_REACH_DOWN
   local y_max = location.y + MAX_REACH_UP
   return self:_create_adjacent_columns(location, y_min, y_max)
end

-- return all the locations that can reach the block at point
function MiningService:get_adjacent_for_destination_block(point)
   local y_min = point.y - MAX_REACH_UP
   local y_max

   if radiant.terrain.is_terrain(point + Point3.unit_y) then
      -- block above is terrain, can't mine down on it
      y_max = point.y
   else
      -- we can strike down on the adjacent block if nothing is on top of it
      y_max = point.y + MAX_REACH_DOWN
   end

   return self:_create_adjacent_columns(point, y_min, y_max)
end

function MiningService:_transform_cubes_in_region(region, cube_transform)
   local transformed_region = Region3()
   
   for cube in region:each_cube() do
      local transformed_cube = cube_transform(cube)
      transformed_region:add_cube(transformed_cube)
   end

   return transformed_region
end

function MiningService:_get_aligned_cube(cube)
   local min, max = cube.min, cube.max
   local aligned_min = Point3(
         math.floor(min.x / XZ_ALIGN) * XZ_ALIGN,
         math.floor(min.y / Y_ALIGN)  * Y_ALIGN,
         math.floor(min.z / XZ_ALIGN) * XZ_ALIGN
      )
   local aligned_max = Point3(
         math.ceil(max.x / XZ_ALIGN) * XZ_ALIGN,
         math.ceil(max.y / Y_ALIGN)  * Y_ALIGN,
         math.ceil(max.z / XZ_ALIGN) * XZ_ALIGN
      )
   return Cube3(aligned_min, aligned_max)
end

function MiningService:_create_adjacent_columns(point, y_min, y_max, remove_blocks_with_terrain)
   local region = Region3()
   local block = Point3()

   local add_xz_column = function(x, z, y_min, y_max)
      for y = y_min, y_max do
         block:set(x, y, z)
         if remove_blocks_with_terrain and radiant.terrain.is_terrain(block) then
            -- don't add
         else
            region:add_point(block)
         end
      end
   end

   add_xz_column(point.x-1, point.z,   y_min, y_max)
   add_xz_column(point.x+1, point.z,   y_min, y_max)
   add_xz_column(point.x,   point.z-1, y_min, y_max)
   add_xz_column(point.x,   point.z+1, y_min, y_max)

   return region
end

return MiningService
