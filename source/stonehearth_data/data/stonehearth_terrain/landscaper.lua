local HeightMap = require 'height_map'
local MathFns = require 'math.math_fns'
local GaussianRandom = require 'math.gaussian_random'
local Wavelet = require 'wavelet.wavelet'
local WaveletFns = require 'wavelet.wavelet_fns'
local Point3 = _radiant.csg.Point3

local Landscaper = class()

local tree_mod_name = 'stonehearth_trees'

local oak = 'oak_tree'
local juniper = 'juniper_tree'
local tree_types = { oak, juniper }

local small = 'small'
local medium = 'medium'
local large = 'large'
local tree_sizes = { small, medium, large }

function Landscaper:__init()
end

-- not ready, just testing density function
function Landscaper:place_trees(zone_map, terrain_info)
   local wavelet_levels = 3
   local freq_scaling_coeff = 0.4
   local grid_spacing = 16
   local perturbation_dist = grid_spacing/4
   local tree_map_width = zone_map.width / grid_spacing
   local tree_map_height = zone_map.height / grid_spacing
   local tree_map = HeightMap(tree_map_width, tree_map_height)

   tree_map:process_map(function () return GaussianRandom.generate(0, 3) end)

   WaveletFns.shape_height_map(tree_map, freq_scaling_coeff, wavelet_levels)

   local grid_offset_x = grid_spacing/2
   local grid_offset_y = grid_spacing/2
   local tree_name, value, elevation
   local i, j, x, y, perturbation_x, perturbation_y

   for j=1, tree_map_height do
      for i=1, tree_map_width do
         value = tree_map:get(i, j)

         -- reduce probability on mountains
         if value > 0 and math.random() < value then
            tree_name = self:random_tree(oak)
            perturbation_x = math.random(-perturbation_dist, perturbation_dist)
            perturbation_y = math.random(-perturbation_dist, perturbation_dist)
            x = (i-1)*grid_spacing + grid_offset_x + perturbation_x
            y = (j-1)*grid_spacing + grid_offset_y + perturbation_y

            elevation = zone_map:get(x, y)
            if elevation <= terrain_info.tree_line then
               self:_place_tree(tree_name, x, y)
            end
         end
      end
   end
end

function Landscaper:place_trees_old(zone_map)
   local num_forests = 15
   local i
   for i=1, num_forests do
      self:place_forest(zone_map)
   end
end

function Landscaper:place_forest(zone_map)
   local max_trees = 20
   local margin_size = 4
   local width = zone_map.width
   local height = zone_map.height
   local min_x = 1 + margin_size
   local min_z = 1 + margin_size
   local max_x = width - margin_size
   local max_z = height - margin_size
   local i, x, z, tree_name
   local tree_type
   local num_trees

   tree_type = self:random_tree_type()
   num_trees = math.random(1, max_trees)

   x = math.random(min_x, max_x)
   z = math.random(min_z, max_z)

   for i=1, num_trees do
      tree_name = self:random_tree(tree_type)
      x, z = self:_next_coord(x, z, min_x, min_z, max_x, max_z)
      self:_place_tree(tree_name, x, z)
   end
end

function Landscaper:random_tree(tree_type, tree_size)
   if tree_type == nil then
      tree_type = self:random_tree_type()
   end
   if tree_size == nil then
      tree_size = self:random_tree_size()
   end
   return self:_get_tree_name(tree_type, tree_size)
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

function Landscaper:_next_coord(x, z, min_x, min_z, max_x, max_z)
   local dx, dz, next_x, next_z

   dx, dz = self:_next_delta(7, 32)

   next_x = x + dx
   if not MathFns.in_bounds(next_x, min_x, max_x) then
      -- no longer uniformly distributed, and biases away from boundary
      next_x = x - dx
      assert(MathFns.in_bounds(next_x, min_x, max_x))
   end

   next_z = z + dz
   if not MathFns.in_bounds(next_z, min_z, max_z) then
      -- no longer uniformly distributed, and biases away from boundary
      next_z = z - dz
      assert(MathFns.in_bounds(next_z, min_z, max_z))
   end

   return next_x, next_z
end

function Landscaper:_next_delta(min_dist, max_dist)
   local min_dist_squared = min_dist*min_dist
   local max_dist_squared = max_dist*max_dist
   local dx, dz, dist_squared

   while true do
      dx = math.random(-max_dist, max_dist)
      dz = math.random(-max_dist, max_dist)
      dist_squared = dx*dx + dz*dz

      if MathFns.in_bounds(dist_squared, min_dist_squared, max_dist_squared) then
         break
      end
   end

   return dx, dz
end

function Landscaper:_place_tree(tree_name, x, z)
   return self:_place_item(tree_mod_name, tree_name, x, z)
end

function Landscaper:_place_item(mod, name, x, z)
   local entity = radiant.entities.create_entity(mod, name)
   radiant.terrain.place_entity(entity, Point3(x, 1, z))
end

function Landscaper:_get_tree_name(tree_type, tree_size)
   return tree_size .. '_' .. tree_type
end

return Landscaper
