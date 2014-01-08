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
local rabbit_name = mod_prefix .. 'rabbit'
local generic_vegetaion_name = "vegetation"
local boulder_name = "boulder"

local Landscaper = class()

function Landscaper:__init(terrain_info, tile_width, tile_height, rng, async)
   if async == nil then async = false end

   self._terrain_info = terrain_info
   self._tile_width = tile_width
   self._tile_height = tile_height
   self._rng = rng
   self._async = async

   -- assert that macroblocks are an integer multiple of the perturbation cell size
   -- no longer support arbitrary alignments in anticipation of future features
   local grid_spacing = 16
   self._feature_cell_size = grid_spacing
   self._perturbation_grid = PerturbationGrid(tile_width, tile_height, grid_spacing, self._rng)

   local feature_map_width, feature_map_height = self._perturbation_grid:get_dimensions()
   self._feature_map = Array2D(feature_map_width, feature_map_height)

   self._noise_map_buffer = Array2D(self._feature_map.width, self._feature_map.height)
   self._density_map_buffer = Array2D(self._feature_map.width, self._feature_map.height)
end

function Landscaper:clear_feature_map()
   self._feature_map:clear(nil)
end

function Landscaper:get_feature_map()
   return self._feature_map:clone()
end

function Landscaper:get_feature_cell_size()
   return self._feature_cell_size
end

function Landscaper:is_forest_feature(feature_name)
   if feature_name == nil then return false end
   if is_tree_name(feature_name) then return true end
   if feature_name == generic_vegetaion_name then return true end
   return false
end

function Landscaper:place_flora(tile_map, world_offset_x, world_offset_y)
   assert(tile_map.width == self._tile_width and tile_map.height == self._tile_height)
   if world_offset_x == nil then world_offset_x = 0 end
   if world_offset_y == nil then world_offset_y = 0 end

   local place_item = function(uri, x, y)
      local entity = radiant.entities.create_entity(uri)
      -- switch from lua height_map base 1 coordinates to c++ base 0 coordinates
      -- swtich from tile coordinates to world coordinates
      radiant.terrain.place_entity(entity, Point3(x+world_offset_x-1, 1, y+world_offset_y-1))
      self:_set_random_facing(entity)
      return entity
   end

   self:_place_trees(tile_map, place_item)
   self:_yield()
   self:_place_berry_bushes(tile_map, place_item)
   self:_yield()
   self:_place_flowers(tile_map, place_item)
   self:_yield()
end

function Landscaper:_place_trees(tile_map, place_item)
   local rng = self._rng
   local terrain_info = self._terrain_info
   local feature_map = self._feature_map
   local perturbation_grid = self._perturbation_grid
   local large_tree_threshold = 25
   local medium_tree_threshold = 6
   local small_tree_threshold = 0
   local max_trunk_radius = 3
   local ground_radius = 2
   local normal_tree_density = 0.8
   local small_tree_density = 0.5
   local noise_map = self._noise_map_buffer
   local density_map = self._density_map_buffer
   local i, j, x, y, tree_type, tree_name, occupied, value, elevation, terrain_type

   local noise_fn = function(i, j)
      local mean = 0
      local std_dev = 100

      if noise_map:is_boundary(i, j) then
         -- soften discontinuities at tile boundaries
         mean = mean - 20
      end

      local x, y = perturbation_grid:get_unperturbed_coordinates(i, j)
      local elevation = tile_map:get(x, y)

      local terrain_type = terrain_info:get_terrain_type(elevation)

      if terrain_type == TerrainType.Mountains then
         --mean = mean + 0
         std_dev = std_dev * 0.30
      elseif terrain_type == TerrainType.Grassland then
         local grassland_info = self._terrain_info[TerrainType.Grassland]

         if elevation == grassland_info.max_height then
            -- sparse groves with large trees in the middle
            mean = mean - 5
            --std_dev = std_dev * 1.00
         else
            -- avoid depressions
            mean = mean - 75
            --std_dev = std_dev * 1.00
         end
      else
         -- TerrainType.Foothills
         local foothills_info = self._terrain_info[TerrainType.Foothills]

         if elevation == foothills_info.max_height then
            -- lots of continuous forest with low variance
            mean = mean + 30
            std_dev = std_dev * 0.30
         else
            -- start transition to grasslands
            mean = mean + 5
            std_dev = std_dev * 0.30
         end
      end

      return rng:get_gaussian(mean, std_dev)
   end

   noise_map:fill(noise_fn)
   FilterFns.filter_2D_0125(density_map, noise_map, noise_map.width, noise_map.height, 10)

   for j=1, density_map.height do
      for i=1, density_map.width do
         value = density_map:get(i, j)

         if value >= small_tree_threshold then
            occupied = feature_map:get(i, j) ~= nil

            if not occupied then
               x, y = perturbation_grid:get_perturbed_coordinates(i, j, max_trunk_radius)

               if self:_is_flat(tile_map, x, y, ground_radius) then
                  elevation = tile_map:get(x, y)
                  tree_type = self:_get_tree_type(elevation)

                  if tree_type ~= nil then 
                     terrain_type = self._terrain_info:get_terrain_type(elevation)

                     if     value >= large_tree_threshold  then tree_name = get_tree_name(tree_type, large)
                     elseif value >= medium_tree_threshold then tree_name = get_tree_name(tree_type, medium)
                     else                                       tree_name = get_tree_name(tree_type, small)
                     end

                     if value >= medium_tree_threshold or terrain_type == TerrainType.Mountains then
                        -- place (at most) a single tree in the cell
                        if rng:get_real(0, 1) < normal_tree_density then
                           place_item(tree_name, x, y)
                           feature_map:set(i, j, tree_name)
                        else
                           -- mark thinned out forest as occupied
                           feature_map:set(i, j, generic_vegetaion_name)
                        end
                      else
                        -- place multiple small trees in the cell
                        -- TODO: clean up this code
                        local w, h, factor, nested_grid_spacing, exclusion_radius, placed
                        x, y, w, h = perturbation_grid:get_cell_bounds(i, j)

                        factor = 0.5
                        exclusion_radius = 2
                        
                        nested_grid_spacing = math.floor(perturbation_grid.grid_spacing * factor)

                        -- inner loop lambda definition - bad for performance?
                        local try_place_item = function(x, y)
                           place_item(tree_name, x, y)
                           return true
                        end

                        placed = self:_place_dense_items(tile_map, x, y, w, h, nested_grid_spacing, exclusion_radius, small_tree_density, try_place_item)
                        if placed then
                           feature_map:set(i, j, tree_name)
                        else
                           -- mark thinned out forest as occupied
                           feature_map:set(i, j, generic_vegetaion_name)
                        end
                     end
                  end
               end
            end
         end
      end
   end
end

function Landscaper:_place_berry_bushes(tile_map, place_item)
   local rng = self._rng
   local terrain_info = self._terrain_info
   local feature_map = self._feature_map
   local perturbation_grid = self._perturbation_grid
   local noise_map = self._noise_map_buffer
   local density_map = self._density_map_buffer
   local item_spacing = math.floor(perturbation_grid.grid_spacing*0.33)
   local item_density = 0.90
   local i, j, x, y, w, h, rows, columns, occupied, value, placed

   local try_place_item = function(x, y)
      local elevation = tile_map:get(x, y)
      local terrain_type = terrain_info:get_terrain_type(elevation)
      if terrain_type == TerrainType.Mountains then
         return false
      end
      place_item(berry_bush_name, x, y)
      return true
   end

   local noise_fn = function(i, j)
      local mean = -45
      local std_dev = 30

      -- berries grow near forests
      local feature = feature_map:get(i, j)
      if is_tree_name(feature) then
         mean = mean + 100
      end

      -- berries grow on foothills ring near mountains
      local x, y = perturbation_grid:get_unperturbed_coordinates(i, j)
      local elevation = tile_map:get(x, y)

      local terrain_type = terrain_info:get_terrain_type(elevation)

      if terrain_type == TerrainType.Foothills then
         local foothills_info = terrain_info[TerrainType.Foothills]
         if elevation == foothills_info.max_height then
            mean = mean + 50
         end
      end

      return rng:get_gaussian(mean, std_dev)
   end

   noise_map:fill(noise_fn)
   FilterFns.filter_2D_050(density_map, noise_map, noise_map.width, noise_map.height, 6)

   for j=1, density_map.height do
      for i=1, density_map.width do
         value = density_map:get(i, j)

         if value > 0 then
            occupied = self._feature_map:get(i, j) ~= nil

            if not occupied then
               x, y, w, h = perturbation_grid:get_cell_bounds(i, j)
               rows, columns = self:_random_berry_pattern()

               placed = self:_place_pattern(tile_map, x, y, w, h, rows, columns, item_spacing, item_density, 1, try_place_item)
               if placed then
                  self._feature_map:set(i, j, berry_bush_name)
               end
            end
         end
      end
   end
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

function Landscaper:_place_flowers(tile_map, place_item)
   local rng = self._rng
   local terrain_info = self._terrain_info
   local feature_map = self._feature_map
   local perturbation_grid = self._perturbation_grid
   local grid_spacing = perturbation_grid.grid_spacing
   local exclusion_radius = 1
   local ground_radius = 1
   local noise_map = self._noise_map_buffer
   local density_map = self._density_map_buffer
   local i, j, x, y, occupied, value, elevation, terrain_type

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

   noise_map:fill(noise_fn)
   FilterFns.filter_2D_025(density_map, noise_map, noise_map.width, noise_map.height, 8)

   for j=1, density_map.height do
      for i=1, density_map.width do
         value = density_map:get(i, j)

         if value > 0 then
            occupied = feature_map:get(i, j) ~= nil

            if not occupied then
               if rng:get_int(1, 100) <= value then
                  x, y = perturbation_grid:get_perturbed_coordinates(i, j, exclusion_radius)

                  if self:_is_flat(tile_map, x, y, ground_radius) then
                     elevation = tile_map:get(x, y)
                     terrain_type = terrain_info:get_terrain_type(elevation)

                     if terrain_type == TerrainType.Grassland then
                        place_item(pink_flower_name, x, y)
                        feature_map:set(i, j, pink_flower_name)

                        -- randomly toss in rabbits with the flowers
                        -- RABBIT WANDER ACTION CURRENTLY BROKEN
                        -- if rng:get_real(0, 1) < 0.10 then
                        --    x, y = perturbation_grid:get_perturbed_coordinates(i, j, 0)
                        --    place_item(rabbit_name, x, y)
                        -- end
                     end
                  end
               end
            end
         end
      end
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

function Landscaper:_get_tree_type(elevation)
   local rng = self._rng
   local mountains_juniper_chance = 0.25
   local high_foothills_juniper_chance = 0.8
   local low_foothills_juniper_chance = 0.2

   local terrain_info = self._terrain_info

   --if elevation > terrain_info.tree_line then return nil end

   local terrain_type = terrain_info:get_terrain_type(elevation)

   if terrain_type == TerrainType.Grassland then return oak end

   if terrain_type == TerrainType.Mountains then
      -- this logic doesn't belong here
      if rng:get_real(0, 1) < mountains_juniper_chance then
         return juniper
      else
         return nil
      end
   end

   local foothills_info = terrain_info[TerrainType.Foothills]

   if elevation >= foothills_info.max_height then
      if rng:get_real(0, 1) < high_foothills_juniper_chance then
         return juniper
      else
         return oak
      end
   end

   if elevation >= foothills_info.max_height-foothills_info.step_size then
      if rng:get_real(0, 1) < low_foothills_juniper_chance then
         return juniper
      else
         return oak
      end
   end
end

function Landscaper:random_tree_type()
   local roll = self._rng:get_int(1, #tree_types)
   return tree_types[roll]
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

function Landscaper:place_boulders(region3_boxed, tile_map)
   local boulder_region
   local exclusion_radius = 8
   local grid_width, grid_height = self._perturbation_grid:get_dimensions()
   local elevation, i, j, x, y

   -- no boulders on edge of map since they can get cut off
   region3_boxed:modify(function(region3)
      for j=2, grid_height-1 do
         for i=2, grid_width-1 do
            x, y = self._perturbation_grid:get_perturbed_coordinates(i, j, exclusion_radius)

            elevation = tile_map:get(x, y)

            if self:_should_place_boulder(elevation) then
               boulder_region = self:_create_boulder(x, y, elevation)
               region3:add_region(boulder_region)
               self._feature_map:set(i, j, boulder_name)
            end
         end
      end
   end)

   self:_yield()
end

function Landscaper:_should_place_boulder(elevation)
   local rng = self._rng
   local terrain_type = self._terrain_info:get_terrain_type(elevation)
   local mountain_boulder_probability = 0.02
   local foothills_boulder_probability = 0.02
   local grassland_boulder_probability = 0.02

   -- drive this from a table later
   if terrain_type == TerrainType.Mountains then
      return rng:get_real(0, 1) <= mountain_boulder_probability
   end

   if terrain_type == TerrainType.Foothills then
      return rng:get_real(0, 1) <= foothills_boulder_probability
   end

   if terrain_type == TerrainType.Grassland then
      return rng:get_real(0, 1) <= grassland_boulder_probability
   end

   return false
end

function Landscaper:_create_boulder(x, y, elevation)
   local terrain_info = self._terrain_info
   local terrain_type = terrain_info:get_terrain_type(elevation)
   local step_size = terrain_info[terrain_type].step_size
   local boulder_region = Region3()
   local boulder_center = Point3(x, elevation, y)
   local i, j, half_width, half_length, half_height, boulder, chunk

   half_width, half_length, half_height = self:_get_boulder_dimensions(terrain_type)

   -- -step_size to make sure boulder reaches floor of terrain when on ledge
   -- +1 to make center of mass appear above ground
   boulder = Cube3(Point3(x-half_width, elevation-step_size, y-half_length),
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
   local half_length, half_width, half_height, aspect_ratio

   if     terrain_type == TerrainType.Mountains then half_width = rng:get_int(4, 9)
   elseif terrain_type == TerrainType.Foothills then half_width = rng:get_int(3, 6)
   elseif terrain_type == TerrainType.Grassland then half_width = rng:get_int(1, 3)
   else return nil, nil, nil
   end

   half_height = half_width+1 -- make boulder look like its sitting slightly above ground
   half_length = half_width
   aspect_ratio = rng:get_gaussian(1, 0.15)

   if rng:get_real(0, 1) <= 0.50 then half_width = MathFns.round(half_width*aspect_ratio)
   else                          half_length = MathFns.round(half_length*aspect_ratio)
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
