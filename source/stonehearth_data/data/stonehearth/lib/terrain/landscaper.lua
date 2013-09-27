local TerrainType = require 'lib.terrain.terrain_type'
local HeightMap = require 'lib.terrain.height_map'
local MathFns = require 'lib.terrain.math.math_fns'
local GaussianRandom = require 'lib.terrain.math.gaussian_random'
local FilterFns = require 'lib.terrain.filter.filter_fns'
local Wavelet = require 'lib.terrain.wavelet.wavelet'
local WaveletFns = require 'lib.terrain.wavelet.wavelet_fns'
local Point3 = _radiant.csg.Point3

local Landscaper = class()

local tree_mod_name = 'stonehearth'

local oak = 'oak_tree'
local juniper = 'juniper_tree'
local tree_types = { oak, juniper }

local small = 'small'
local medium = 'medium'
local large = 'large'
local tree_sizes = { small, medium, large }

function Landscaper:__init(terrain_info)
   self.terrain_info = terrain_info
end

-- not ready, just testing density function
function Landscaper:place_trees(zone_map, world_offset_x, world_offset_y)
   if world_offset_x == nil then world_offset_x = 0 end
   if world_offset_y == nil then world_offset_y = 0 end

   local wavelet_levels = 2
   local freq_scaling_coeff = 0.2
   local grid_spacing = 32
   local perturbation_dist = grid_spacing/2 - 4
   local tree_map_width = zone_map.width / grid_spacing
   local tree_map_height = zone_map.height / grid_spacing
   local tree_map = HeightMap(tree_map_width, tree_map_height)
   local noise_map = HeightMap(tree_map_width, tree_map_height)

   --self:_fill_noise_map(tree_map)
   --tree_map:print()
   --WaveletFns.shape_height_map(tree_map, freq_scaling_coeff, wavelet_levels)

   self:_fill_noise_map(noise_map)
   --noise_map:print()
   FilterFns.filter_2D_025(tree_map, noise_map, tree_map_width, tree_map_height)

   --radiant.log.info('')
   --tree_map:print()

   local terrain_info = self.terrain_info
   local grid_offset_x = grid_spacing/2
   local grid_offset_y = grid_spacing/2
   local tree_type, tree_name, value, elevation
   local i, j, x, y, perturbation_x, perturbation_y

   for j=1, tree_map_height do
      for i=1, tree_map_width do
         value = tree_map:get(i, j)

         if value > 0 then
            perturbation_x = math.random(-perturbation_dist, perturbation_dist)
            perturbation_y = math.random(-perturbation_dist, perturbation_dist)
            x = (i-1)*grid_spacing + grid_offset_x + perturbation_x
            y = (j-1)*grid_spacing + grid_offset_y + perturbation_y

            elevation = zone_map:get(x, y)
            tree_type = self:_get_tree_type(elevation)

            if tree_type ~= nil then 
               if value <= 10 then     tree_name = get_tree_name(tree_type, small)
               elseif value <= 25 then tree_name = get_tree_name(tree_type, medium)
               else                    tree_name = get_tree_name(tree_type, large)
               end

               self:_place_tree(tree_name, world_offset_x + x, world_offset_y + y)
            end
         end
      end
   end
end

function Landscaper:_fill_noise_map(height_map)
   local mean = 10
   local std_dev = 100
   local i, j, value

   for j=1, height_map.height do
      for i=1, height_map.width do
         if on_edge(height_map, i, j) then
            -- discourage forests from running into zone boundaries
            value = -10
         else
            value = GaussianRandom.generate(mean, std_dev)
         end
         height_map:set(i, j, value)
      end
   end
end

function Landscaper:_get_tree_type(elevation)
   local terrain_info = self.terrain_info

   if elevation > terrain_info.tree_line then return nil end
   if elevation <= terrain_info[TerrainType.Plains].max_height then return oak end
   if elevation > terrain_info[TerrainType.Foothills].max_height then
      -- mountains have reduced forest density
      return juniper
      --if math.random() < 0.5 then return juniper end
      --return nil
   end
   return self:random_tree_type()
end

function on_edge(height_map, x, y)
   if x == 1 or y == 1 then return true end
   if x == height_map.width or y == height_map.height then return true end
   return false
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

function Landscaper:_place_tree(tree_name, x, z)
   return self:_place_item(tree_name, x, z)
end

function Landscaper:_place_item(uri, x, z)
   local entity = radiant.entities.create_entity(uri)
   -- switch from lua height_map coordinates to cpp coordinates
   radiant.terrain.place_entity(entity, Point3(x-1, 1, z-1))
end

function get_tree_name(tree_type, tree_size)
   return tree_mod_name .. '.' .. tree_size .. '_' .. tree_type
end

return Landscaper
