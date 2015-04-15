local TerrainInfo = require 'services.server.world_generation.terrain_info'
local Array2D = require 'services.server.world_generation.array_2D'
local FilterFns = require 'services.server.world_generation.filter.filter_fns'
local PerturbationGrid = require 'services.server.world_generation.perturbation_grid'
local Timer = require 'services.server.world_generation.timer'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local log = radiant.log.create_logger('world_generation')

local mod_name = 'stonehearth'
local mod_prefix = mod_name .. ':'

local oak = 'oak_tree'
local juniper = 'juniper_tree'
local tree_types = { oak, juniper }

local small = 'small'
local medium = 'medium'
local large = 'large'
local ancient = 'ancient'
local tree_sizes = { small, medium, large, ancient }

local berry_bush_name = mod_prefix .. 'berry_bush'
local generic_vegetaion_name = "vegetation"

local water_shallow = 'water_1'
local water_deep = 'water_2'

local Landscaper = class()

-- TODO: refactor this class into smaller pieces
function Landscaper:__init(terrain_info, rng)
   self._terrain_info = terrain_info
   self._tile_width = self._terrain_info.tile_size
   self._tile_height = self._terrain_info.tile_size
   self._feature_size = self._terrain_info.feature_size
   self._rng = rng

   self._noise_map_buffer = nil
   self._density_map_buffer = nil

   self._perturbation_grid = PerturbationGrid(self._tile_width, self._tile_height, self._feature_size, self._rng)

   self._water_table = {
      water_1 = 5,
      water_2 = 10
   }

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
   if self:is_tree_name(feature_name) then return true end
   if feature_name == generic_vegetaion_name then return true end
   return false
end

function Landscaper:is_water_feature(feature_name)
   local result = self._water_table[feature_name] ~= nil
   return result
end

function Landscaper:place_flora(tile_map, feature_map, tile_offset_x, tile_offset_y)
   local place_item = function(uri, x, y)
      local entity = radiant.entities.create_entity(uri)
      self:_set_random_facing(entity)

      local elevation = tile_map:get(x, y)
      local world_x, world_z = self:_to_world_coordinates(x, y, tile_offset_x, tile_offset_y)
      local location = radiant.terrain.get_point_on_terrain(Point3(world_x, elevation, world_z))

      if radiant.terrain.is_standable(entity, location) then
         radiant.terrain.place_entity(entity, location)
      end

      return entity
   end

   self:place_features(tile_map, feature_map, place_item)
end

function Landscaper:mark_water_bodies(elevation_map, feature_map)
   local rng = self._rng
   local terrain_info = self._terrain_info
   local noise_map, density_map = self:_get_filter_buffers(feature_map.width, feature_map.height)

   local noise_fn = function(i, j)
      local mean = 0
      local std_dev = 100

      local terrain_means = {
         plains = -20,
         foothills = -10,
         mountains = -10
      }

      local boundary = noise_map:is_boundary(i, j) or not self:_is_flat(elevation_map, i, j, 1)

      if boundary then
         -- don't allow lakes near boundaries
         mean = -200
      else
         local elevation = elevation_map:get(i, j)
         local terrain_type, step = terrain_info:get_terrain_type_and_step(elevation)
         mean = terrain_means[terrain_type]
      end

      return rng:get_gaussian(mean, std_dev)
   end

   noise_map:fill(noise_fn)
   FilterFns.filter_2D_0125(density_map, noise_map, noise_map.width, noise_map.height, 10)

   local old_feature_map = Array2D(feature_map.width, feature_map.height)

   for j=1, density_map.height do
      for i=1, density_map.width do
         local occupied = feature_map:get(i, j) ~= nil

         if not occupied then
            local value = density_map:get(i, j)

            if value > 0 then
               local old_value = feature_map:get(i, j)
               old_feature_map:set(i, j, old_value)
               feature_map:set(i, j, water_shallow)
            end
         end
      end
   end

   self:_remove_juts(feature_map)
   self:_remove_ponds(feature_map, old_feature_map)
   self:_add_deep_water(feature_map)
end

function Landscaper:_remove_juts(feature_map)
   -- just 1 pass currently
   -- could record fixups and recursively recheck the 8 adjacents
   for j=2, feature_map.height-1 do
      for i=2, feature_map.width-1 do
         if self:_is_peninsula(feature_map, i, j) then
            feature_map:set(i, j, water_shallow)
         end
      end
   end
end

function Landscaper:_remove_ponds(feature_map, old_feature_map)
   for j=2, feature_map.height-1 do
      for i=2, feature_map.width-1 do
         local feature_name = feature_map:get(i, j)

         if self:is_water_feature(feature_name) then
            local has_water_neighbor = false

            feature_map:each_neighbor(i, j, false, function(value)
                  if self:is_water_feature(value) then
                     has_water_neighbor = true
                     return true -- stop iteration
                  end
               end)

            if not has_water_neighbor then
               local old_value = old_feature_map:get(i, j)
               feature_map:set(i, j, old_value)
            end
         end
      end
   end
end

function Landscaper:_add_deep_water(feature_map)
   for j=2, feature_map.height-1 do
      for i=2, feature_map.width-1 do
         local feature_name = feature_map:get(i, j)

         if self:is_water_feature(feature_name) then
            local surrounded_by_water = true

            feature_map:each_neighbor(i, j, true, function(value)
                  if not self:is_water_feature(value) then
                     surrounded_by_water = false
                     return true -- stop iteration
                  end
               end)

            if surrounded_by_water then
               feature_map:set(i, j, water_deep)
            end
         end
      end
   end
end

function Landscaper:place_water_bodies(tile_region, tile_map, feature_map, tile_offset_x, tile_offset_y)
   local water_region = Region3()

   feature_map:visit(function(value, i, j)
         if not self:is_water_feature(value) then
            return
         end

         local depth = self._water_table[value]
         local x, y, w, h = self._perturbation_grid:get_cell_bounds(i, j)

         -- use the center of the cell to get the elevation because the edges may have been detailed
         local cx, cy = x + math.floor(w*0.5), y + math.floor(h*0.5)
         local lake_top = tile_map:get(cx, cy)
         local lake_bottom = lake_top - depth

         local world_x, world_z = self:_to_world_coordinates(x, y, tile_offset_x, tile_offset_y)
         local cube = Cube3(
               Point3(world_x, lake_bottom, world_z),
               Point3(world_x + w, lake_top, world_z + h)
            )

         tile_region:subtract_cube(cube)

         water_region:add_cube(cube)
      end)

   water_region:optimize_by_merge('place water bodies')

   return water_region
end

function Landscaper:mark_trees(elevation_map, feature_map)
   local rng = self._rng
   local terrain_info = self._terrain_info
   local normal_tree_density = 0.8
   local mountains_size_modifier = 0.10
   local mountains_density_base = 0.40
   local mountains_density_peaks = 0.10
   local noise_map, density_map = self:_get_filter_buffers(feature_map.width, feature_map.height)
   local tree_name, tree_type, tree_size, occupied, value, elevation, terrain_type, step, tree_density

   local noise_fn = function(i, j)
      local mean = 0
      local std_dev = 100

      if noise_map:is_boundary(i, j) then
         -- soften discontinuities at tile boundaries
         mean = mean - 20
      end

      local elevation = elevation_map:get(i, j)
      local terrain_type, step = terrain_info:get_terrain_type_and_step(elevation)

      if terrain_type == 'mountains' then
         if step <= 2 then
            -- place a ring of trees around mountains by making mountains attractors
            -- trees on mountains themselves will be thinned out below
            mean = mean + 50
            std_dev = 0
         else
            std_dev = std_dev * 0.30
         end
      elseif terrain_type == 'plains' then
         if step == 2 then
            -- sparse groves with large trees in the middle
            mean = mean - 5
         else
            -- avoid depressions
            mean = mean - 300
            std_dev = 0
         end
      else
         -- 'foothills'
         if step == 2 then
            -- lots of continuous forest with low variance
            mean = mean + 5
            std_dev = std_dev * 0.30
         else
            -- start transition to plains
            --mean = mean + 5
            std_dev = std_dev * 0.30
         end
      end

      return rng:get_gaussian(mean, std_dev)
   end

   noise_map:fill(noise_fn)
   FilterFns.filter_2D_0125(density_map, noise_map, noise_map.width, noise_map.height, 10)

   for j=1, density_map.height do
      for i=1, density_map.width do
         occupied = feature_map:get(i, j) ~= nil

         if not occupied then
            value = density_map:get(i, j)

            if value > 0 then
               elevation = elevation_map:get(i, j)
               terrain_type, step = terrain_info:get_terrain_type_and_step(elevation)
               tree_type = self:_get_tree_type(terrain_type, step)

               -- TODO: clean up this code
               if terrain_type ~= 'mountains' then
                  tree_density = normal_tree_density
               else
                  -- forests on mountains are shorter and thinner
                  value = value * mountains_size_modifier

                  if step == 1 then
                     tree_density = mountains_density_base
                  else
                     tree_density = mountains_density_peaks
                  end
               end

               tree_size = self:_get_tree_size(value)
               tree_name = get_tree_name(tree_type, tree_size)

               if terrain_type ~= 'mountains' then
                  if tree_size ~= small then
                     if rng:get_real(0, 1) >= tree_density then
                        -- thin out the tree but still mark as occupied
                        tree_name = generic_vegetaion_name
                     end
                  end
                  feature_map:set(i, j, tree_name)
               else
                  if rng:get_real(0, 1) < tree_density then
                     feature_map:set(i, j, tree_name)
                  end
               end
            end
         end
      end
   end
end

function Landscaper:_get_tree_type(terrain_type, step)
   local rng = self._rng
   local terrain_info = self._terrain_info
   local high_foothills_juniper_chance = 0.75
   local low_foothills_juniper_chance = 0.25

   if terrain_type == 'plains' then
      return oak
   end

   if terrain_type == 'mountains' then
      return juniper
   end

   -- terrain_type == 'foothills'
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
   local rng = self._rng

   local large_tree_threshold = 20
   local medium_tree_threshold = 4

   if value >= large_tree_threshold then
      if rng:get_int(1, 10) == 10 then
         return ancient
      else 
         return large
      end
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

   -- hack to prevent dense trees on mountains
   local center = self._feature_size * 0.5
   local elevation = tile_map:get(x+center, y+center)
   local terrain_type = self._terrain_info:get_terrain_type(elevation)
   if terrain_type == 'mountains' then
      return self:_place_normal_tree(feature_name, i, j, tile_map, place_item)
   end

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
   local rng = self._rng
   local perturbation_grid = self._perturbation_grid
   local x, y

   x, y = perturbation_grid:get_perturbed_coordinates(i, j, max_trunk_radius)

   if self:_is_flat(tile_map, x, y, ground_radius) then
      place_item(feature_name, x, y)
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
      if self:is_tree_name(feature) then
         mean = mean + 100
      end

      -- berries grow on foothills ring near mountains
      -- don't do this until we can ladder up foothills, need to reduce non-foothills density as well
      -- local elevation = elevation_map:get(i, j)
      -- local terrain_type, step = terrain_info:get_terrain_type_and_step(elevation)

      -- if terrain_type == 'foothills' and step == 2 then
      --    mean = mean + 20
      -- end

      return rng:get_gaussian(mean, std_dev)
   end

   noise_map:fill(noise_fn)
   FilterFns.filter_2D_050(density_map, noise_map, noise_map.width, noise_map.height, 6)

   for j=1, density_map.height do
      for i=1, density_map.width do
         value = density_map:get(i, j)

         if value > 0 then
            occupied = feature_map:get(i, j) ~= nil

            if not occupied then
               elevation = elevation_map:get(i, j)
               terrain_type = terrain_info:get_terrain_type(elevation)

               if terrain_type ~= 'mountains' then
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
      if terrain_type == 'mountains' then
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
         mean = mean - 20
      end

      -- flowers grow away from forests
      local feature = feature_map:get(i, j)
      if self:is_tree_name(feature) then
         mean = mean - 200
      end

      local elevation = elevation_map:get(i, j)
      local terrain_type, step = terrain_info:get_terrain_type_and_step(elevation)

      if terrain_type == 'plains' and step == 1 then
         -- avoid depressions
         mean = mean - 200
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
                  elevation = elevation_map:get(i, j)
                  terrain_type = terrain_info:get_terrain_type(elevation)

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

   tile_map:visit_block(start_x, start_y, block_width, block_height, function(value)
         if value ~= height then
            is_flat = false
            -- return true to terminate iteration
            return true
         end
      end)

   return is_flat
end

function Landscaper:_is_peninsula(feature_map, i, j)
   local feature_name = feature_map:get(i, j)
   if self:is_water_feature(feature_name) then
      return false
   end

   local water_count = 0

   feature_map:each_neighbor(i, j, false, function(value)
         if self:is_water_feature(value) then
            water_count = water_count + 1
         end
      end)

   local result = water_count == 3
   return result
end

function Landscaper:_set_random_facing(entity)
   entity:add_component('mob'):turn_to(90*self._rng:get_int(0, 3))
end

function get_tree_name(tree_type, tree_size)
   return mod_prefix .. tree_size .. '_' .. tree_type
end

function Landscaper:is_tree_name(feature_name)
   if feature_name == nil then return false end
   -- may need to be more robust later
   local index = feature_name:find('_tree', -5)
   return index ~= nil
end

-- switch from lua height_map base 1 coordinates to c++ base 0 coordinates
-- swtich from tile coordinates to world coordinates
function Landscaper:_to_world_coordinates(x, y, tile_offset_x, tile_offset_y)
   local world_x = x-1+tile_offset_x
   local world_z = y-1+tile_offset_y
   return world_x, world_z
end

return Landscaper
