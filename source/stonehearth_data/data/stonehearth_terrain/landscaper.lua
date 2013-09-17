local MathFns = radiant.mods.require('/stonehearth_terrain/math/math_fns.lua')
local GaussianRandom = radiant.mods.require('/stonehearth_terrain/math/gaussian_random.lua')
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

function Landscaper:place_trees(zone_map)
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
