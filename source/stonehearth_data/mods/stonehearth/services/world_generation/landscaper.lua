local TerrainType = require 'services.world_generation.terrain_type'
local TerrainInfo = require 'services.world_generation.terrain_info'
local Array2D = require 'services.world_generation.array_2D'
local MathFns = require 'services.world_generation.math.math_fns'
local GaussianRandom = require 'services.world_generation.math.gaussian_random'
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

local Landscaper = class()

function Landscaper:__init(terrain_info, tile_width, tile_height)
   self.terrain_info = terrain_info
   self.tile_width = tile_width
   self.tile_height = tile_height

   local grid_spacing = 16
   self.flora_perturbation_grid = PerturbationGrid(tile_width, tile_height, grid_spacing)

   local feature_map_width, feature_map_height = self.flora_perturbation_grid:get_dimensions()
   self.feature_map = Array2D(feature_map_width, feature_map_height)

   self.noise_map_buffer = Array2D(self.feature_map.width, self.feature_map.height)
   self.density_map_buffer = Array2D(self.feature_map.width, self.feature_map.height)
end

function Landscaper:place_flora(tile_map, world_offset_x, world_offset_y)
   assert(tile_map.width == self.tile_width and tile_map.height == self.tile_height)
   if world_offset_x == nil then world_offset_x = 0 end
   if world_offset_y == nil then world_offset_y = 0 end

   local place_item = function(uri, x, y)
      local entity = radiant.entities.create_entity(uri)
      -- switch from lua height_map base 1 coordinates to c++ base 0 coordinates
      -- swtich from tile coordinates to world coordinates
      radiant.terrain.place_entity(entity, Point3(x+world_offset_x-1, 1, y+world_offset_y-1))
      _set_random_facing(entity)
      return entity
   end

   self.feature_map:clear(nil)
   self:_place_trees(tile_map, place_item)
   self:_place_berry_bushes(tile_map, place_item)
   self:_place_flowers(tile_map, place_item)
end

-- TODO: refactor
function Landscaper:_place_trees(tile_map, place_item)
   local small_tree_threshold = 10
   local medium_tree_threshold = 35
   local max_trunk_radius = 3
   local ground_radius = 2
   local noise_map = self.noise_map_buffer
   local density_map = self.density_map_buffer
   local default_tree_type = self:random_tree_type() -- default tree type for this tile
   local i, j, x, y, tree_type, tree_name, occupied, value, elevation, mean_density
   
   if tile_map.terrain_type == TerrainType.Grassland then
      mean_density = 0
   else
      mean_density = 0
   end

   self:_fill_noise_map(noise_map, self.feature_map, mean_density, -10, -10)
   FilterFns.filter_2D_025(density_map, noise_map, noise_map.width, noise_map.height, 8)

   for j=1, density_map.height do
      for i=1, density_map.width do
         value = density_map:get(i, j)

         if value > 0 then
            occupied = self.feature_map:get(i, j) ~= nil

            if not occupied then
               x, y = self.flora_perturbation_grid:get_perturbed_coordinates(i, j, max_trunk_radius)

               if self:_is_flat(tile_map, x, y, ground_radius) then
                  elevation = tile_map:get(x, y)
                  tree_type = self:_get_tree_type(elevation, default_tree_type)

                  if tree_type ~= nil then 
                     if value <= small_tree_threshold      then tree_name = get_tree_name(tree_type, small)
                     elseif value <= medium_tree_threshold then tree_name = get_tree_name(tree_type, medium)
                     else                                       tree_name = get_tree_name(tree_type, large)
                     end

                     place_item(tree_name, x, y)
                     self.feature_map:set(i, j, tree_name)
                  end
               end
            end
         end
      end
   end
end

-- TODO: refactor
function Landscaper:_place_flowers(tile_map, place_item)
   local grid_spacing = self.flora_perturbation_grid.grid_spacing
   local exclusion_radius = 1
   local ground_radius = 1
   local noise_map = self.noise_map_buffer
   local density_map = self.density_map_buffer
   local threshold = 10
   local i, j, x, y, occupied, value, elevation, terrain_type

   self:_fill_noise_map(noise_map, self.feature_map, 10, -50, -50)
   FilterFns.filter_2D_025(density_map, noise_map, noise_map.width, noise_map.height, 8)

   for j=1, density_map.height do
      for i=1, density_map.width do
         value = density_map:get(i, j)

         if value > threshold then
            occupied = self.feature_map:get(i, j) ~= nil

            if not occupied then
               if math.random(100) <= value then
                  x, y = self.flora_perturbation_grid:get_perturbed_coordinates(i, j, exclusion_radius)

                  if self:_is_flat(tile_map, x, y, ground_radius) then
                     elevation = tile_map:get(x, y)
                     terrain_type = self.terrain_info:get_terrain_type(elevation)

                     if terrain_type == TerrainType.Grassland then
                        place_item(pink_flower_name, x, y)
                        self.feature_map:set(i, j, pink_flower_name)
                     end
                  end
               end
            end
         end
         -- For dense flowers
         -- occupied = self.feature_map:get(i, j) ~= nil
         -- value = density_map:get(i, j)

         -- if not occupied and value > 0 then
         --    x, y, w, h = self.flora_perturbation_grid:get_cell_bounds(i, j)

         --    if     value > 36 then factor = 0.33
         --    elseif value > 25 then factor = 0.5
         --    else                   factor = 1
         --    end

         --    nested_grid_spacing = math.floor(grid_spacing * factor)

         --    self:_place_dense_items(tile_map, x, y, w, h, nested_grid_spacing, exclusion_radius, pink_flower_name, place_item)
         --    self.feature_map:set(i, j, pink_flower_name)
         -- end
      end
   end
end

-- TODO: refactor
function Landscaper:_place_berry_bushes(tile_map, place_item)
   local grid_spacing = self.flora_perturbation_grid.grid_spacing
   local exclusion_radius = 1
   local ground_radius = 0
   local noise_map = self.noise_map_buffer
   local density_map = self.density_map_buffer
   local threshold = 60
   local i, j, x, y, occupied, value, placed

   local try_place_item = function(x, y)
      local elevation = tile_map:get(x, y)
      local terrain_type = self.terrain_info:get_terrain_type(elevation)
      if terrain_type == TerrainType.Mountains then
         return false
      end
      place_item(berry_bush_name, x, y)
      return true
   end

   self:_fill_noise_map(noise_map, self.feature_map, -15, 100, 0)
   FilterFns.filter_2D_025(density_map, noise_map, noise_map.width, noise_map.height, 8)

   for j=1, density_map.height do
      for i=1, density_map.width do
         value = density_map:get(i, j)

         if value > threshold then
            occupied = self.feature_map:get(i, j) ~= nil

            if not occupied then
               local w, h, factor, probability, nested_grid_spacing
               x, y, w, h = self.flora_perturbation_grid:get_cell_bounds(i, j)

               factor = 0.33
               probability = value * 0.01
               --probability = 1.0
               --exclusion_radius = -1
               
               nested_grid_spacing = math.floor(grid_spacing * factor)

               -- TODO: replace this with pattern stamps
               placed = self:_place_dense_items(tile_map, x, y, w, h, nested_grid_spacing, exclusion_radius, probability, try_place_item)
               if placed then
                  self.feature_map:set(i, j, berry_bush_name)
               end
            end
         end
      end
   end
end

function Landscaper:_place_dense_items(tile_map, cell_origin_x, cell_origin_y, cell_width, cell_height,
                                       grid_spacing, exclusion_radius, probability, try_place_item)

   -- consider removing this memory allocation
   local perturbation_grid = PerturbationGrid(cell_width, cell_height, grid_spacing)
   local grid_width, grid_height = perturbation_grid:get_dimensions()
   local i, j, dx, dy, x, y, result
   local placed = false

   for j=1, grid_height do
      for i=1, grid_width do
         if math.random() < probability then
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

function Landscaper:_fill_noise_map(height_map, exclusion_map, mean, flora_exclusion_value, boundary_exclusion_value)
   local std_dev = 100
   local i, j, value

   for j=1, height_map.height do
      for i=1, height_map.width do
         if height_map:is_boundary(i, j) then
            -- discourage discontinuities at tile boundaries
            value = mean + boundary_exclusion_value
         else
            if exclusion_map:get(i, j) == nil then
               -- unoccupied
               value = GaussianRandom.generate(mean, std_dev)
            else
               -- occupied, discourage flora from growing here
               value = flora_exclusion_value
            end
         end
         height_map:set(i, j, value)
      end
   end
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

   tile_map:visit_block(start_x, start_y, block_width, block_height,
      function (value)
         if value == height then return true end
         is_flat = false
         return false
      end
   )

   return is_flat
end

function Landscaper:_get_tree_type(elevation, default_tree_type)
   local terrain_info = self.terrain_info

   if elevation > terrain_info.tree_line then return nil end

   local terrain_type = terrain_info:get_terrain_type(elevation)

   if terrain_type == TerrainType.Mountains then return juniper end
   if terrain_type == TerrainType.Grassland then return oak end

   if elevation < terrain_info[TerrainType.Foothills].max_height then
      return oak
   else
      -- highest level of foothills can be junipers
      return default_tree_type
   end
end

function Landscaper:random_tree(tree_type, tree_size)
   if tree_type == nil then
      tree_type = self:random_tree_type()
   end
   if tree_size == nil then
      tree_size = self:random_tree_size()
   end
   return get_tree_name(tree_type, tree_size)
end

function Landscaper:random_tree_type()
   local roll = math.random(1, #tree_types)
   return tree_types[roll]
end

-- young and old trees are less common
function Landscaper:random_tree_size()
   local std_dev = #tree_sizes*0.33 -- edge values about half as likely as center value
   local roll = GaussianRandom.generate_int(1, #tree_sizes, std_dev)
   return tree_sizes[roll]
end

function _set_random_facing(entity)
   entity:add_component('mob'):turn_to(90*math.random(0, 3))
end

function get_tree_name(tree_type, tree_size)
   return mod_prefix .. tree_size .. '_' .. tree_type
end

-----

function Landscaper:place_boulders(region3_boxed, tile_map)
   local boulder_region
   local grid_spacing = 32
   local exclusion_radius = 4
   local perturbation_grid = PerturbationGrid(tile_map.width, tile_map.height, grid_spacing)
   local boulder_map_width, boulder_map_height = perturbation_grid:get_dimensions()
   local elevation, i, j, x, y

   -- no boulders on edge of map since they can get cut off
   region3_boxed:modify(function(region3)
      for j=2, boulder_map_height-1 do
         for i=2, boulder_map_width-1 do
            x, y = perturbation_grid:get_perturbed_coordinates(i, j, exclusion_radius)

            elevation = tile_map:get(x, y)

            if self:_should_place_boulder(elevation) then
               boulder_region = self:_create_boulder(x, y, elevation)
               region3:add_region(boulder_region)
            end
         end
      end
   end)
end

function Landscaper:_should_place_boulder(elevation)
   local terrain_type = self.terrain_info:get_terrain_type(elevation)
   local mountain_boulder_probability = 0.09
   local foothills_boulder_probability = 0.06
   local grassland_boulder_probability = 0.03

   -- drive this from a table later
   if terrain_type == TerrainType.Mountains then
      return math.random() <= mountain_boulder_probability
   end

   if terrain_type == TerrainType.Foothills then
      return math.random() <= foothills_boulder_probability
   end

   if terrain_type == TerrainType.Grassland then
      return math.random() <= grassland_boulder_probability
   end

   return false
end

function Landscaper:_create_boulder(x, y, elevation)
   local chunk_probability = 1.0
   local terrain_info = self.terrain_info
   local terrain_type = terrain_info:get_terrain_type(elevation)
   local step_size = terrain_info[terrain_type].step_size
   local boulder_region = Region3()
   local half_width, half_length, half_height

   half_width, half_length, half_height = self:_get_boulder_dimensions(terrain_type)

   -- -step_size to make sure boulder reaches floor of terrain when on ledge
   -- +1 to make center of mass appear above ground
   local boulder = Cube3(Point3(x-half_width, elevation-step_size, y-half_length),
                         Point3(x+half_width, elevation+half_height, y+half_length),
                         Terrain.BOULDER)

   boulder_region:add_cube(boulder)

   if math.random() <= chunk_probability then
      local chunk = self:_get_boulder_chunk(Point3(x, elevation, y),
                                            half_width, half_height, half_length)
      boulder_region:subtract_cube(chunk)
   end

   return boulder_region
end

-- dimensions are distances from the center
function Landscaper:_get_boulder_dimensions(terrain_type)
   local half_length, half_width, half_height, aspect_ratio

   if     terrain_type == TerrainType.Mountains then half_width = math.random(6, 12)
   elseif terrain_type == TerrainType.Foothills then half_width = math.random(4, 8)
   elseif terrain_type == TerrainType.Grassland then half_width = math.random(2, 4)
   else return nil, nil, nil
   end

   half_height = half_width+1 -- make boulder look like its sitting slightly above ground
   half_length = half_width
   aspect_ratio = GaussianRandom.generate(1, 0.15)

   if math.random() <= 0.50 then half_width = MathFns.round(half_width*aspect_ratio)
   else                          half_length = MathFns.round(half_length*aspect_ratio)
   end

   return half_width, half_length, half_height
end

function Landscaper:_get_boulder_chunk(boulder_center, half_width, half_height, half_length)
   -- randomly find a corner (in cube space, not region space)
   local sign_x = math.random(0, 1)*2 - 1
   local sign_y = math.random(0, 1)*2 - 1

   local corner1 = boulder_center + Point3(sign_x*half_width,
                                           half_height,
                                           sign_y*half_length)

   -- length of the chunk as a percent of the length of the boulder
   local chunk_length_percent = math.random(1, 4) * 0.25
   local chunk_depth = math.floor(half_height*0.5)

   -- pick an edge for the chunk
   local corner2
   if math.random() <= 0.50 then
      corner2 = corner1 + Point3(-sign_x * 2*half_width * chunk_length_percent,
                                 -chunk_depth,
                                 -sign_y * chunk_depth)
   else
      corner2 = corner1 + Point3(-sign_x * chunk_depth,
                                 -chunk_depth,
                                 -sign_y * 2*half_length * chunk_length_percent)
   end

   return ConstructCube3(corner1, corner2, 0)
end

return Landscaper
