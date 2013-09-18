local MathFns = require 'math.math_fns'
local GaussianRandom = require 'math.gaussian_random'
local Point3 = _radiant.csg.Point3

local Landscaper = class()

local folder = '/stonehearth_trees/entities/'

local oak = 'oak_tree'
local juniper = 'juniper_tree'
local tree_types = { oak, juniper }

local small = 'small'
local medium = 'medium'
local large = 'large'
local tree_sizes = { small, medium, large }

function Landscaper:__init()
end

function Landscaper:place_trees(zone_map)
   local num_forests = 15
   local i
   for i=1, num_forests, 1 do
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
   local i, x, z, tree
   local tree_type
   local num_trees

   tree_type = self:random_tree_type()
   num_trees = math.random(1, max_trees)

   x = math.random(1, width)
   z = math.random(1, height)

   for i=1, num_trees, 1 do
      tree = self:random_tree(tree_type)
      x, z = self:_next_coord(x, z, min_x, min_z, max_x, max_z)
      self:_place_item(tree, x, z)
   end
end

function Landscaper:random_tree(tree_type, tree_size)
   if tree_type == nil then
      tree_type = self:random_tree_type()
   end
   if tree_size == nil then
      tree_size = self:random_tree_size()
   end
   return self:_get_tree_resource_name(tree_type, tree_size)
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

   while true do
      dx, dz = self:_next_delta(7, 32)

      next_x = x + dx
      next_z = z + dz

      if MathFns.in_bounds(next_x, min_x, max_x) and
         MathFns.in_bounds(next_z, min_z, max_z) then
         break
      end
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

function Landscaper:_place_item(name, x, z)
   local entity = radiant.entities.create_entity(name)
   radiant.terrain.place_entity(entity, Point3(x, 1, z))
end

function Landscaper:_get_tree_resource_name(tree_type, tree_size)
   return folder .. tree_type .. '/' .. tree_size .. '_' .. tree_type
end

return Landscaper
