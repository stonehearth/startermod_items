local TerrainType = require 'services.world_generation.terrain_type'
local TerrainInfo = require 'services.world_generation.terrain_info'
local Array2D = require 'services.world_generation.array_2D'
local MathFns = require 'services.world_generation.math.math_fns'
local GaussianRandom = require 'services.world_generation.math.gaussian_random'
local FilterFns = require 'services.world_generation.filter.filter_fns'

local Terrain = _radiant.om.Terrain
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

local Landscaper = class()

local oak = 'oak_tree'
local juniper = 'juniper_tree'
local tree_types = { oak, juniper }

local small = 'small'
local medium = 'medium'
local large = 'large'
local tree_widths = { small, medium, large }

function Landscaper:__init(terrain_info)
   self.terrain_info = terrain_info
end

function Landscaper:place_trees(zone_map, world_offset_x, world_offset_y)
   if world_offset_x == nil then world_offset_x = 0 end
   if world_offset_y == nil then world_offset_y = 0 end

   local small_tree_threshold = 10
   local medium_tree_threshold = 35
   local grid_spacing = 18
   local max_trunk_radius = 3
   local perturbation_dist = _calc_perturbation_dist(grid_spacing, max_trunk_radius)
   local tree_map_width, tree_map_height, grid_offset_x, grid_offset_y =
            _calc_perturbation_map_size_and_offset(zone_map, grid_spacing)
   local tree_map = Array2D(tree_map_width, tree_map_height)
   local noise_map = Array2D(tree_map_width, tree_map_height)

   self:_fill_noise_map(noise_map)
   FilterFns.filter_2D_025(tree_map, noise_map, tree_map_width, tree_map_height, 8)

   local terrain_info = self.terrain_info
   local default_tree_type = self:random_tree_type() -- default tree type for this zone
   local tree_type, tree_name, value, elevation
   local i, j, x, y

   for j=1, tree_map_height do
      for i=1, tree_map_width do
         value = tree_map:get(i, j)

         if value > 0 then
            x, y = _perturbation_to_zone_coordinates(i, j, grid_spacing,
                      grid_offset_x, grid_offset_y, perturbation_dist)

            if self:_is_flat(zone_map, x, y, 2) then
               elevation = zone_map:get(x, y)
               tree_type = self:_get_tree_type(elevation, default_tree_type)

               if tree_type ~= nil then 
                  if value <= small_tree_threshold      then tree_name = get_tree_name(tree_type, small)
                  elseif value <= medium_tree_threshold then tree_name = get_tree_name(tree_type, medium)
                  else                                       tree_name = get_tree_name(tree_type, large)
                  end

                  local entity
                  entity = self:_place_item(tree_name, world_offset_x + x, world_offset_y + y)
                  
                  -- set a random facing for the tree
                  entity:add_component('mob'):turn_to(90*math.random(0, 3))
               end
            end
         end
      end
   end
end

function _calc_perturbation_dist(grid_spacing, exclusion_radius)
   return math.floor(grid_spacing/2 - exclusion_radius)
end

function _calc_perturbation_map_size_and_offset(zone_map, grid_spacing)
   local perturbation_map_width = math.floor(zone_map.width / grid_spacing)
   local perturbation_map_height = math.floor(zone_map.height / grid_spacing)
   local remainder_x = zone_map.width % perturbation_map_width
   local remainder_y = zone_map.height % perturbation_map_height
   local grid_offset_x = math.floor(grid_spacing/2 + remainder_x)
   local grid_offset_y = math.floor(grid_spacing/2 + remainder_y)

   return perturbation_map_width, perturbation_map_height, grid_offset_x, grid_offset_y
end

-- takes coordinates i, j from the perturbation grid and returns perturbed zone coordinates
function _perturbation_to_zone_coordinates(i, j, grid_spacing, grid_offset_x, grid_offset_y, perturbation_dist)
   local perturbation_x = math.random(-perturbation_dist, perturbation_dist)
   local perturbation_y = math.random(-perturbation_dist, perturbation_dist)

   local x = (i-1)*grid_spacing + grid_offset_x + perturbation_x
   local y = (j-1)*grid_spacing + grid_offset_y + perturbation_y

   return x, y
end

function Landscaper:_fill_noise_map(height_map)
   local mean = 0
   local std_dev = 100
   local i, j, value

   for j=1, height_map.height do
      for i=1, height_map.width do
         if height_map:is_boundary(i, j) then
            -- discourage forests from being discontinuous at zone boundaries
            value = mean-20
         else
            value = GaussianRandom.generate(mean, std_dev)
         end
         height_map:set(i, j, value)
      end
   end
end

-- checks if the rectangular region centered around x,y is flat
function Landscaper:_is_flat(zone_map, x, y, distance)
   local start_x, start_y = zone_map:bound(x-distance, y-distance)
   local end_x, end_y = zone_map:bound(x+distance, y+distance)
   local block_width = end_x - start_x + 1
   local block_height = end_y - start_y + 1
   local height = zone_map:get(x, y)
   local is_flat = true

   zone_map:visit_block(start_x, start_y, block_width, block_height,
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

   if terrain_type == TerrainType.Grassland then return oak end
   if terrain_type == TerrainType.Mountains then return juniper end

   return default_tree_type
end

function Landscaper:random_tree(tree_type, tree_width)
   if tree_type == nil then
      tree_type = self:random_tree_type()
   end
   if tree_width == nil then
      tree_width = self:random_tree_width()
   end
   return get_tree_name(tree_type, tree_width)
end

function Landscaper:random_tree_type()
   local roll = math.random(1, #tree_types)
   return tree_types[roll]
end

-- young and old trees are less common
function Landscaper:random_tree_width()
   local std_dev = #tree_widths*0.33 -- edge values about half as likely as center value
   local roll = GaussianRandom.generate_int(1, #tree_widths, std_dev)
   return tree_widths[roll]
end

function Landscaper:_place_item(uri, x, z)
   local entity = radiant.entities.create_entity(uri)
   -- switch from lua height_map coordinates to cpp coordinates
   radiant.terrain.place_entity(entity, Point3(x-1, 1, z-1))
   return entity
end

function get_tree_name(tree_type, tree_width)
   return 'stonehearth:' .. tree_width .. '_' .. tree_type
end

-----

function Landscaper:place_boulders(region3_boxed, zone_map)
   local region3 = region3_boxed:modify()
   local boulder_region

   local grid_spacing = 32
   local perturbation_dist = grid_spacing/2 - 4
   local boulder_map_width = zone_map.width / grid_spacing
   local boulder_map_height = zone_map.height / grid_spacing
   local boulder_map_width, boulder_map_height, grid_offset_x, grid_offset_y =
            _calc_perturbation_map_size_and_offset(zone_map, grid_spacing)

   local elevation, i, j, x, y

   -- no boulders on edge of map since they can get cut off
   for j=2, boulder_map_height-1 do
      for i=2, boulder_map_width-1 do
         x, y = _perturbation_to_zone_coordinates(i, j, grid_spacing,
                   grid_offset_x, grid_offset_y, perturbation_dist)

         elevation = zone_map:get(x, y)

         if self:_should_place_boulder(elevation) then
            boulder_region = self:_create_boulder(x, y, elevation)
            region3:add_region(boulder_region)
         end
      end
   end

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
   local chunk_probability = 0.50
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

   if     terrain_type == TerrainType.Mountains then half_width = math.random(8, 16)
   elseif terrain_type == TerrainType.Foothills then half_width = math.random(6, 10)
   elseif terrain_type == TerrainType.Grassland then half_width = math.random(4, 8)
   else return nil, nil, nil
   end

   half_height = half_width+1 -- make boulder look like its sitting slightly above ground
   half_length = half_width
   aspect_ratio = GaussianRandom.generate(1, 0.25)

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

   return _create_cube(corner1, corner2)
end

function _create_cube(point1, point2)
   local min_point = Point3(math.min(point1.x, point2.x),
                            math.min(point1.y, point2.y),
                            math.min(point1.z, point2.z))
   local max_point = Point3(math.max(point1.x, point2.x),
                            math.max(point1.y, point2.y),
                            math.max(point1.z, point2.z))
   return Cube3(min_point, max_point)
end

return Landscaper
