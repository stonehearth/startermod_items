local constants = require 'constants'
local mining_lib = require 'lib.mining.mining_lib'
local LootTable = require 'lib.loot_table.loot_table'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('mining')

MiningService = class()

local MAX_REACH_UP = 3
local MAX_REACH_DOWN = 1

function MiningService:initialize()
   self._enable_insta_mine = radiant.util.get_config('enable_insta_mine', false)

   self._sv = self.__saved_variables:get_data()

   self:_init_loot_tables()

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

-- Dig an arbitary region. Region is defined in world space.
function MiningService:dig_region(player_id, region)
   if self._enable_insta_mine then
      self:_insta_mine(region)
      return nil
   end

   -- only merge zones in the same xz slice
   local inflated_region = region:inflated(Point3(1, 0, 1))

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
      selected_zone = self:create_mining_zone(player_id)
      town:add_mining_zone(selected_zone)
   end

   self:add_region_to_zone(selected_zone, region)

   -- merge the other zones into the surviving zone and destroy them
   for _, zone in pairs(mergable_zones) do
      self:merge_zones(selected_zone, zone)
   end

   return selected_zone
end

-- Dig down, quantized to the 4x5x4 mining cells.
-- Region is defined in world space.
function MiningService:dig_down(player_id, region)
   local aligned_region = self:_transform_cubes_in_region(region, function(cube)
         return self:_get_aligned_cube(cube)
      end)
   self:dig_region(player_id, aligned_region)
end

-- Dig out, quantized to the 4x5x4 cells, but preserve the ceiling.
-- Region is defined in world space.
function MiningService:dig_out(player_id, region)
   local aligned_region = self:_transform_cubes_in_region(region, function(cube)
         local aligned_cube = self:_get_aligned_cube(cube)
         local min, max = aligned_cube.min, aligned_cube.max

         -- drop the max y level by 1 to preserve the ceiling
         return Cube3(
               min,
               Point3(max.x, max.y-1, max.z)
            )
      end)
   self:dig_region(player_id, aligned_region)
end

-- Dig up, quantized to the 4x5x4 cells.
-- Region is defined in world space.
function MiningService:dig_up(player_id, region)
   local aligned_region = self:_transform_cubes_in_region(region, function(cube)
         local aligned_cube = self:_get_aligned_cube(cube)
         local min, max = aligned_cube.min, aligned_cube.max

         -- ok this region API definition is weird
         return Cube3(
               Point3(min.x, min.y-1, min.z),
               Point3(max.x, max.y-1, max.z)
            )
      end)
   self:dig_region(player_id, aligned_region)
end

-- Explicitly create a mining zone.
function MiningService:create_mining_zone(player_id)
   local mining_zone = radiant.entities.create_entity('stonehearth:mining_zone')

   mining_zone:add_component('unit_info')
      :set_player_id(player_id)

   return mining_zone
end

-- Explicitly add a region to a mining zone.
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

-- Merges zone2 into zone1, and destroys zone2.
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

-- Chooses the best point to mine when standing on from.
function MiningService:resolve_point_of_interest(from, mining_zone)
   local location = radiant.entities.get_world_grid_location(mining_zone)
   local destination_component = mining_zone:add_component('destination')
   local destination_region = destination_component:get_region():get()
   local reserved_region = destination_component:get_reserved():get()

   -- get the reachable region in local coordinates to the zone
   local reachable_region = self:get_reachable_region(from - location)
   local eligible_region = reachable_region - reserved_region
   local eligible_destination_region = eligible_region:intersected(destination_region)
   local max

   if eligible_destination_region:empty() then
      return nil
   end

   -- pick any highest point in the region
   for cube in eligible_destination_region:each_cube() do
      if not max or cube.max.y > max.y then
         max = cube.max
      end
   end

   -- strip one off the max to get terrain coordinates of block
   -- then convert to world coordinates
   local poi = max - Point3.one + location
   assert(poi ~= from - Point3.unit_y)
   return poi
end

-- Returns a region containing the poi and all blocks that support it.
-- Currently just the blocks below the poi, but could be useful when trying to
-- clear out unsupported floors.
function MiningService:get_reserved_region_for_poi(poi, from, mining_zone)
   local location = radiant.entities.get_world_grid_location(mining_zone)
   local mining_zone_component = mining_zone:add_component('stonehearth:mining_zone')
   local zone_region = mining_zone_component:get_region():get()
   local poi = poi - location
   local from = from - location

   local cube = Cube3(poi, poi + Point3.one)
   if poi.y > from.y then
      cube.min.y = from.y
   end
   local proposed_region = Region3(cube)
   local reserved_region = zone_region:intersected(proposed_region)
   -- by convention, all input and output values in the mining service are in world coordiantes
   reserved_region:translate(location)
   return reserved_region
end

-- Return all the locations that can be reached from point.
function MiningService:get_reachable_region(location)
   local y_min = location.y - MAX_REACH_DOWN
   local y_max = location.y + MAX_REACH_UP
   local region = self:_create_adjacent_columns(location, y_min, y_max)
   return region
end

-- Return all the locations that can reach the block at point.
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

   local region = self:_create_adjacent_columns(point, y_min, y_max, function(block)
         return not radiant.terrain.is_terrain(block)
      end)
   return region
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
   return mining_lib.get_aligned_cube(cube, constants.mining.XZ_CELL_SIZE, constants.mining.Y_CELL_SIZE)
end

function MiningService:_create_adjacent_columns(point, y_min, y_max, block_filter)
   local region = Region3()
   local block = Point3()

   local add_xz_column = function(x, z, y_min, y_max)
      for y = y_min, y_max do
         block:set(x, y, z)
         if not block_filter or block_filter(block) then
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

function MiningService:_init_loot_tables()
   local json = radiant.resources.load_json('/stonehearth/services/server/mining/mining_loot_tables.json')
   self._loot_tables = {}

   for block_kind, loot_json in pairs(json) do
      local loot_table = LootTable()
      loot_table:load_from_json(loot_json)
      self._loot_tables[block_kind] = loot_table
   end
end

function MiningService:roll_loot(block_kind)
   local loot_table = self._loot_tables[block_kind]
   local uris = loot_table and loot_table:roll_loot() or {}
   return uris
end

-- temporary location for this function
function MiningService:mine_point(point)
   radiant.terrain.subtract_point(point)
end

function MiningService:_insta_mine(region)
   local terrain_region = radiant.terrain.intersect_region(region)

   for point in terrain_region:each_point() do
      self:mine_point(point)
   end
end

return MiningService
