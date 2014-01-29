local TerrainType = require 'services.world_generation.terrain_type'
local TerrainInfo = require 'services.world_generation.terrain_info'
local Array2D = require 'services.world_generation.array_2D'
local MathFns = require 'services.world_generation.math.math_fns'
local FilterFns = require 'services.world_generation.filter.filter_fns'
local PerturbationGrid = require 'services.world_generation.perturbation_grid'
local log = radiant.log.create_logger('world_generation')

local Terrain = _radiant.om.Terrain
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local ConstructCube3 = _radiant.csg.ConstructCube3
local Region3 = _radiant.csg.Region3

local mod_name = 'stonehearth'
local mod_prefix = mod_name .. ':'

local oak = 'oak_tree'
local juniper = 'juniper_tree'
local tree_types = { oak, juniper }

local small = 'small'
local medium = 'medium'
local large = 'large'
local tree_sizes = { small, medium, large }

local pink_flower_name = mod_prefix .. 'pink_flower'
local berry_bush_name = mod_prefix .. 'berry_bush'
local generic_vegetaion_name = "vegetation"
local boulder_name = "boulder"

local Landscaper = class()

-- TODO: refactor this class into smaller pieces
function Landscaper:__init(terrain_info, rng, async)
   if async == nil then async = false end

   self._terrain_info = terrain_info
   self._tile_width = self._terrain_info.tile_size
   self._tile_height = self._terrain_info.tile_size
   self._feature_size = self._terrain_info.feature_size
   self._rng = rng
   self._async = async

   self._boulder_probabilities = {
      [TerrainType.plains]    = 0.02,
      [TerrainType.foothills] = 0.02,
      [TerrainType.mountains] = 0.02
   }

   self._boulder_size = {
      [TerrainType.plains]    = { min = 1, max = 3 },
      [TerrainType.foothills] = { min = 3, max = 6 },
      [TerrainType.mountains] = { min = 4, max = 9 }
   }

   self._noise_map_buffer = nil
   self._density_map_buffer = nil

   self._perturbation_grid = PerturbationGrid(self._tile_width, self._tile_height, self._feature_size, self._rng)

   self:_initialize_function_table()
end

function Landscaper:_initialize_function_table()
   local tree_name
   local function_table = {}

   for _, tree_type in pairs(tree_types) do
      for _, tree_size in pairs(tree_sizes) do
         tree_name = get_tree_name(tree_type, tree_size)

         if tree_size == small then
            function_table[tree_name] = self._place_small_tree
         else
            function_table[tree_name] = self._place_normal_tree
         end
      end
   end

   function_table[berry_bush_name] = self._place_berry_bush
   function_table[pink_flower_name] = self._place_flower

   -- need a better name for this
   self._function_table = function_table
end

function Landscaper:_get_filter_buffers(width, height)
   if self._noise_map_buffer == nil or
      self._noise_map_buffer.width ~= width or 
      self._noise_map_buffer.height ~= height then

      self._noise_map_buffer = Array2D(width, height)
      self._density_map_buffer = Array2D(width, height)
   end

   assert(self._density_map_buffer.width == self._noise_map_buffer.width)
   assert(self._density_map_buffer.height == self._noise_map_buffer.height)

   return self._noise_map_buffer, self._density_map_buffer
end

function Landscaper:is_forest_feature(feature_name)
   if feature_name == nil then return false end
   if is_tree_name(feature_name) then return true end
   if feature_name == generic_vegetaion_name then return true end
   return false
end

function Landscaper:place_flora(tile_map, feature_map, tile_offset_x, tile_offset_y)
   local place_item = function(uri, x, y)
      local entity = radiant.entities.create_entity(uri)
      -- switch from lua height_map base 1 coordinates to c++ base 0 coordinates
      -- swtich from tile coordinates to world coordinates
      radiant.terrain.place_entity(entity, Point3(x-1+tile_offset_x, 1, y-1+tile_offset_y))
      self:_set_random_facing(entity)
      return entity
   end

   self:place_features(tile_map, feature_map, place_item)
end

function Landscaper:mark_trees(elevation_map, feature_map)
   local rng = self._rng
   local terrain_info = self._terrain_info
   local mountains_juniper_chance = 0.25
   local noise_map, density_map = self:_get_filter_buffers(feature_map.width, feature_map.height)
   local i, j, tree_name, tree_type, tree_size, occupied, value, elevation, terrain_type

   local noise_fn = function(i, j)
      local mean = 0
      local std_dev = 100

      if noise_map:is_boundary(i, j) then
         -- soften discontinuities at tile boundaries
         mean = mean - 20
      end

      local elevation = elevation_map:get(i, j)
      local terrain_type, step = terrain_info:get_terrain_type_and_step(elevation)

      if terrain_type == TerrainType.mountains then
         std_dev = std_dev * 0.30
      elseif terrain_type == TerrainType.plains then
         if step == 2 then
            -- sparse groves with large trees in the middle
            mean = mean - 5
         else
            -- avoid depressions
            mean = mean - 75
         end
      else
         -- TerrainType.foothills
         if step == 2 then
            -- lots of continuous forest with low variance
            mean = mean + 30
            std_dev = std_dev * 0.30
         else
            -- start transition to plains
            --mean = mean + 5
            std_dev = std_dev * 0.30
         end
      end

      return rng:get_gaussian(mean, std_dev)
   end

   noise_map:fill_ij(noise_fn)
   FilterFns.filter_2D_0125(density_map, noise_map, noise_map.width, noise_map.height, 10)

   for j=1, density_map.height do
      for i=1, density_map.width do
         occupied = feature_map:get(i, j) ~= nil

         if not occupied then
            value = density_map:get(i, j)

            if value > 0 then
               elevation = elevation_map:get(i, j)
               tree_type = self:_get_tree_type(elevation)
               terrain_type = terrain_info:get_terrain_type(elevation)

               if terrain_type == TerrainType.mountains then
                  if rng:get_real(0, 1) >= mountains_juniper_chance then
                     tree_type = nil
                  end
               end

               if tree_type ~= nil then 
                  tree_size = self:_get_tree_size(value)
                  tree_name = get_tree_name(tree_type, tree_size)
                  feature_map:set(i, j, tree_name)
               end
            end
         end
      end
   end
end

function Landscaper:_get_tree_type(elevation)
   local rng = self._rng
   local terrain_info = self._terrain_info
   local high_foothills_juniper_chance = 0.8
   local low_foothills_juniper_chance = 0.2

   --if elevation > terrain_info.tree_line then return nil end

   local terrain_type, step = terrain_info:get_terrain_type_and_step(elevation)

   if terrain_type == TerrainType.plains then
      return oak
   end

   if terrain_type == TerrainType.mountains then
      return juniper
   end

   -- terrain_type == TerrainType.foothills
   if step == 2 then 
      if rng:get_real(0, 1) < high_foothills_juniper_chance then
         return juniper
      else
         return oak
      end
   else
      -- step == 1
      if rng:get_real(0, 1) < low_foothills_juniper_chance then
         return juniper
      else
         return oak
      end
   end
end

function Landscaper:_get_tree_size(value)
   local large_tree_threshold = 25
   local medium_tree_threshold = 6

   if value >= large_tree_threshold then
      return large
   end

   if value >= medium_tree_threshold then
      return medium
   end

   return small
end

-- place multiple small trees in the cell
function Landscaper:_place_small_tree(feature_name, i, j, tile_map, place_item)
   local small_tree_density = 0.5
   local exclusion_radius = 2
   local factor = 0.5
   local rng = self._rng
   local perturbation_grid = self._perturbation_grid
   local x, y, w, h, nested_grid_spacing

   x, y, w, h = perturbation_grid:get_cell_bounds(i, j)

   nested_grid_spacing = math.floor(perturbation_grid.grid_spacing * factor)

   -- inner loop lambda definition - bad for performance?
   local try_place_item = function(x, y)
      place_item(feature_name, x, y)
      return true
   end

   self:_place_dense_items(tile_map, x, y, w, h, nested_grid_spacing,
      exclusion_radius, small_tree_density, try_place_item)

   -- if unable to place, should we remark feature map?
end

function Landscaper:_place_normal_tree(feature_name, i, j, tile_map, place_item)
   local max_trunk_radius = 3
   local ground_radius = 2
   local normal_tree_density = 0.8
   local rng = self._rng
   local perturbation_grid = self._perturbation_grid
   local x, y

   x, y = perturbation_grid:get_perturbed_coordinates(i, j, max_trunk_radius)

   if self:_is_flat(tile_map, x, y, ground_radius) then
      if rng:get_real(0, 1) < normal_tree_density then
         place_item(feature_name, x, y)
      end
      -- if tree was thinned out, should we remark feature map?
   end
end

function Landscaper:place_features(tile_map, feature_map, place_item)
   local feature_name, fn

   for j=1, feature_map.height do
      for i=1, feature_map.width do
         feature_name = feature_map:get(i, j)
         fn = self._function_table[feature_name]

         if fn then
            fn(self, feature_name, i, j, tile_map, place_item)
         end
      end
      self:_yield()
   end
end

function Landscaper:mark_berry_bushes(elevation_map, feature_map)
   local rng = self._rng
   local terrain_info = self._terrain_info
   local perturbation_grid = self._perturbation_grid
   local noise_map, density_map = self:_get_filter_buffers(feature_map.width, feature_map.height)
   local value, occupied, elevation, terrain_type

   local noise_fn = function(i, j)
      local mean = -50
      local std_dev = 30

      -- berries grow near forests
      local feature = feature_map:get(i, j)
      if is_tree_name(feature) then
         mean = mean + 100
      end

      -- berries grow on foothills ring near mountains
      local elevation = elevation_map:get(i, j)
      local terrain_type, step = terrain_info:get_terrain_type_and_step(elevation)

      if terrain_type == TerrainType.foothills and step == 2 then
         mean = mean + 50
      end

      return rng:get_gaussian(mean, std_dev)
   end

   noise_map:fill_ij(noise_fn)
   FilterFns.filter_2D_050(density_map, noise_map, noise_map.width, noise_map.height, 6)

   for j=1, density_map.height do
      for i=1, density_map.width do
         value = density_map:get(i, j)

         if value > 0 then
            occupied = feature_map:get(i, j) ~= nil

            if not occupied then
               elevation = elevation_map:get(i, j)
               terrain_type = terrain_info:get_terrain_type(elevation)

               if terrain_type ~= TerrainType.mountains then
                  feature_map:set(i, j, berry_bush_name)
               end
            end
         end
      end
   end
end

function Landscaper:_place_berry_bush(feature_name, i, j, tile_map, place_item)
   local terrain_info = self._terrain_info
   local perturbation_grid = self._perturbation_grid
   local item_spacing = math.floor(perturbation_grid.grid_spacing*0.33)
   local item_density = 0.90
   local x, y, w, h, rows, columns

   local try_place_item = function(x, y)
      local elevation = tile_map:get(x, y)
      local terrain_type = terrain_info:get_terrain_type(elevation)
      if terrain_type == TerrainType.mountains then
         return false
      end
      place_item(feature_name, x, y)
      return true
   end

   x, y, w, h = perturbation_grid:get_cell_bounds(i, j)
   rows, columns = self:_random_berry_pattern()

   self:_place_pattern(tile_map, x, y, w, h, rows, columns, item_spacing, item_density, 1, try_place_item)
end

function Landscaper:_random_berry_pattern()
   local roll = self._rng:get_int(1, 3)

   if roll == 1 then
      return 2, 3
   elseif roll == 2 then
      return 3, 2
   else
      return 2, 2
   end
end

function Landscaper:mark_flowers(elevation_map, feature_map)
   local rng = self._rng
   local terrain_info = self._terrain_info
   local perturbation_grid = self._perturbation_grid
   local noise_map, density_map = self:_get_filter_buffers(feature_map.width, feature_map.height)
   local value, occupied, elevation, terrain_type

   local noise_fn = function(i, j)
      local mean = 0
      local std_dev = 100

      if noise_map:is_boundary(i, j) then
         -- soften discontinuities at tile boundaries
         mean = mean - 50
      end

      -- flowers grow away from forests
      local feature = feature_map:get(i, j)
      if is_tree_name(feature) then
         mean = mean - 50
      end

      return rng:get_gaussian(mean, std_dev)
   end

   noise_map:fill_ij(noise_fn)
   FilterFns.filter_2D_025(density_map, noise_map, noise_map.width, noise_map.height, 8)

   for j=1, density_map.height do
      for i=1, density_map.width do
         value = density_map:get(i, j)

         if value > 0 then
            occupied = feature_map:get(i, j) ~= nil

            if not occupied then
               if rng:get_int(1, 100) <= value then
                  elevation = elevation_map:get(i, j)
                  terrain_type = terrain_info:get_terrain_type(elevation)

                  if terrain_type == TerrainType.plains then
                     feature_map:set(i, j, pink_flower_name)
                  end
               end
            end
         end
      end
   end
end

function Landscaper:_place_flower(feature_name, i, j, tile_map, place_item)
   local exclusion_radius = 1
   local ground_radius = 1
   local perturbation_grid = self._perturbation_grid
   local x, y

   x, y = perturbation_grid:get_perturbed_coordinates(i, j, exclusion_radius)

   if self:_is_flat(tile_map, x, y, ground_radius) then
      place_item(feature_name, x, y)
   end
end

function Landscaper:_place_pattern(tile_map, x, y, w, h, columns, rows, spacing, density, max_empty_positions, try_place_item)
   local rng = self._rng
   local i, j, result
   local x_offset = math.floor((w - spacing*(columns-1)) * 0.5)
   local y_offset = math.floor((h - spacing*(rows-1)) * 0.5)
   local x_start = x + x_offset
   local y_start = y + y_offset
   local num_empty_positions = 0
   local skip
   local placed = false

   for j=1, rows do
      for i=1, columns do
         skip = false
         if num_empty_positions < max_empty_positions then
            if rng:get_real(0, 1) >= density then
               num_empty_positions = num_empty_positions + 1
               skip = true
            end
         end
         if not skip then
            result = try_place_item(x_start + (i-1)*spacing, y_start + (j-1)*spacing)
            if result then
               placed = true
            end
         end
      end
   end

   return placed
end

function Landscaper:_place_dense_items(tile_map, cell_origin_x, cell_origin_y, cell_width, cell_height,
                                       grid_spacing, exclusion_radius, probability, try_place_item)
   local rng = self._rng
   -- consider removing this memory allocation
   local perturbation_grid = PerturbationGrid(cell_width, cell_height, grid_spacing, rng)
   local grid_width, grid_height = perturbation_grid:get_dimensions()
   local i, j, dx, dy, x, y, result
   local placed = false

   for j=1, grid_height do
      for i=1, grid_width do
         if rng:get_real(0, 1) < probability then
            if exclusion_radius >= 0 then
               dx, dy = perturbation_grid:get_perturbed_coordinates(i, j, exclusion_radius)
            else
               dx, dy = perturbation_grid:get_unperturbed_coordinates(i, j)
            end

            -- -1 becuase get_perturbed_coordinates returns base 1 coords and cell_origin is already at 1,1 of cell
            x = cell_origin_x + dx-1
            y = cell_origin_y + dy-1

            result = try_place_item(x, y)
            if result then
               placed = true
            end
         end
      end
   end

   return placed
end

-- checks if the rectangular region centered around x,y is flat
function Landscaper:_is_flat(tile_map, x, y, distance)
   if distance == 0 then return true end

   local start_x, start_y = tile_map:bound(x-distance, y-distance)
   local end_x, end_y = tile_map:bound(x+distance, y+distance)
   local block_width = end_x - start_x + 1
   local block_height = end_y - start_y + 1
   local height = tile_map:get(x, y)
   local is_flat = true

   is_flat = tile_map:visit_block(start_x, start_y, block_width, block_height,
      function (value)
         return value == height
      end
   )

   return is_flat
end

function Landscaper:_set_random_facing(entity)
   entity:add_component('mob'):turn_to(90*self._rng:get_int(0, 3))
end

function get_tree_name(tree_type, tree_size)
   return mod_prefix .. tree_size .. '_' .. tree_type
end

function is_tree_name(feature_name)
   if feature_name == nil then return false end
   -- may need to be more robust later
   local index = feature_name:find('_tree', -5)
   return index ~= nil
end

-----

function Landscaper:mark_boulders(elevation_map, feature_map)
   local elevation

   -- no boulders on edge of map since they may exceed boundaries
   for j=2, feature_map.height-1 do
      for i=2, feature_map.width-1 do
         elevation = elevation_map:get(i, j)

         if self:_should_place_boulder(elevation) then
            feature_map:set(i, j, boulder_name)
         end
      end
   end
end

function Landscaper:place_boulders_internal(region3_boxed, tile_map, feature_map)
   local boulder_region
   local exclusion_radius = 8
   local grid_width, grid_height = self._perturbation_grid:get_dimensions()
   local feature_name, elevation, i, j, x, y

   -- no boulders on edge of map since they can get cut off
   region3_boxed:modify(
      function(region3)
         for j=2, grid_height-1 do
            for i=2, grid_width-1 do
               feature_name = feature_map:get(i, j)

               if feature_name == boulder_name then
                  x, y = self._perturbation_grid:get_perturbed_coordinates(i, j, exclusion_radius)
                  elevation = tile_map:get(x, y)

                  boulder_region = self:_create_boulder(x, y, elevation)
                  region3:add_region(boulder_region)
               end
            end
         end
      end
   )
end

function Landscaper:place_boulders(region3_boxed, tile_map, feature_map)
   self:place_boulders_internal(region3_boxed, tile_map, feature_map)
   self:_yield()
end

function Landscaper:_should_place_boulder(elevation)
   local terrain_type = self._terrain_info:get_terrain_type(elevation)
   local probability = self._boulder_probabilities[terrain_type]

   return self._rng:get_real(0, 1) < probability
end

function Landscaper:_create_boulder(x, y, elevation)
   local terrain_info = self._terrain_info
   local terrain_type = terrain_info:get_terrain_type(elevation)
   local step_size = terrain_info[terrain_type].step_size
   local boulder_region = Region3()
   local boulder_center = Point3(x, elevation, y)
   local i, j, half_width, half_length, half_height, boulder, chunk

   half_width, half_length, half_height = self:_get_boulder_dimensions(terrain_type)

   boulder = Cube3(Point3(x-half_width, elevation-2, y-half_length),
                         Point3(x+half_width, elevation+half_height, y+half_length),
                         Terrain.BOULDER)

   boulder_region:add_cube(boulder)

   -- take out a small chip from each corner of larger boulders
   local avg_length = MathFns.round((2*half_width + 2*half_length) * 0.5)
   if avg_length >= 6 then
      local chip_size = MathFns.round(avg_length * 0.15)
      local chip

      for j=-1, 1, 2 do
         for i=-1, 1, 2 do
            chip = self:_get_boulder_chip(i, j, chip_size, boulder_center,
                                          half_width, half_height, half_length)
            boulder_region:subtract_cube(chip)
         end
      end
   end

   -- remove a big chunk from one corner
   chunk = self:_get_boulder_chunk(boulder_center,
                                   half_width, half_height, half_length)
   boulder_region:subtract_cube(chunk)

   return boulder_region
end

-- dimensions are distances from the center
function Landscaper:_get_boulder_dimensions(terrain_type)
   local rng = self._rng
   local size, half_length, half_width, half_height, aspect_ratio

   size = self._boulder_size[terrain_type]
   half_width = rng:get_int(size.min, size.max)

   half_height = half_width+1 -- make boulder look like its sitting slightly above ground
   half_length = half_width
   aspect_ratio = rng:get_gaussian(1, 0.15)

   if rng:get_real(0, 1) <= 0.50 then
      half_width = MathFns.round(half_width*aspect_ratio)
   else
      half_length = MathFns.round(half_length*aspect_ratio)
   end

   return half_width, half_length, half_height
end

function Landscaper:_get_boulder_chip(sign_x, sign_y, chip_size, boulder_center, half_width, half_height, half_length)
   local corner1 = boulder_center + Point3(sign_x * half_width,
                                           half_height,
                                           sign_y * half_length)
   local corner2 = corner1 + Point3(-sign_x * chip_size,
                                    -chip_size,
                                    -sign_y * chip_size)
   return ConstructCube3(corner1, corner2, 0)
end

function Landscaper:_get_boulder_chunk(boulder_center, half_width, half_height, half_length)
   local rng = self._rng
   -- randomly find a corner (in cube space, not region space)
   local sign_x = rng:get_int(0, 1)*2 - 1
   local sign_y = rng:get_int(0, 1)*2 - 1

   local corner1 = boulder_center + Point3(sign_x * half_width,
                                           half_height,
                                           sign_y * half_length)

   -- length of the chunk as a percent of the length of the boulder
   local chunk_length_percent = rng:get_int(1, 2) * 0.25
   local chunk_depth = math.floor(half_height*0.5)
   local chunk_size

   -- pick an edge for the chunk
   local corner2
   if rng:get_real(0, 1) <= 0.50 then
      -- rounding causes some 75% chunks to look odd (the remaining slice is too narrow)
      -- so floor instead, but make it non-zero so we always have a chunk
      chunk_size = math.floor(2*half_width * chunk_length_percent)
      if chunk_size == 0 then
         chunk_size = 1
      end

      corner2 = corner1 + Point3(-sign_x * chunk_size,
                                 -chunk_depth,
                                 -sign_y * chunk_depth)
   else
      chunk_size = math.floor(2*half_length * chunk_length_percent)
      if chunk_size == 0 then
         chunk_size = 1
      end

      corner2 = corner1 + Point3(-sign_x * chunk_depth,
                                 -chunk_depth,
                                 -sign_y * chunk_size)
   end

   return ConstructCube3(corner1, corner2, 0)
end

function Landscaper:_yield()
   if self._async then
      coroutine.yield()
   end
end

return Landscaper
