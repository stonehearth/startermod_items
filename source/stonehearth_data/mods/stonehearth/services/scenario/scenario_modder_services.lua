local MathFns = require 'services.world_generation.math.math_fns'
local PerturbationGrid = require 'services.world_generation.perturbation_grid'
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('scenario_service')

local ScenarioModderServices = class()

-- Modder API starts here
-- methods and properties starting with _ are not intended for modder use

-- (x, y) is valid from (1, 1) - (properties.size.width, properties.size.length)
function ScenarioModderServices:place_entity(uri, x, y)
   x, y = self:_bounds_check(x, y)

   local entity = radiant.entities.create_entity(uri)

   -- switch from lua height_map base 1 coordinates to c++ base 0 coordinates
   -- swtich from scenario coordinates to world coordinates
   radiant.terrain.place_entity(entity, Point3(x-1 + self._offset_x, 1, y-1 + self._offset_y))
   --self:_set_random_facing(entity)
   return entity
end

function ScenarioModderServices:place_entity_cluster(uri, quantity, entity_radius)
   local rng = self.rng
   local size = self._properties.size
   local grid_spacing = self:_get_perturbation_grid_spacing(size.width, size.length, quantity)
   local grid = PerturbationGrid(size.width, size.length, grid_spacing, self.rng)
   local num_cells_x, num_cells_y = grid:get_dimensions()
   local cells_left = num_cells_x * num_cells_y
   local num_selected = 0
   local x, y, probability

   for j=1, num_cells_y do
      for i=1, num_cells_x do
         -- this algorithm guarantees that each cell has an equal probability of being selected
         probability = (quantity - num_selected) / cells_left

         if rng:get_real(0, 1) < probability then
            x, y = grid:get_perturbed_coordinates(i, j, entity_radius)
            self:place_entity(uri, x, y)
            num_selected = num_selected + 1

            if num_selected == quantity then
               return
            end
         end
         cells_left = cells_left - 1
      end
   end
end


-- Private API starts here
function ScenarioModderServices:__init(rng)
   self.rng = rng
end

function ScenarioModderServices:_set_scenario_properties(properties, offset_x, offset_y)
   self._properties = properties
   self._offset_x = offset_x
   self._offset_y = offset_y
end

function ScenarioModderServices:_bounds_check(x, y)
   local size = self._properties.size

   return MathFns.bound(x, 1, size.width),
          MathFns.bound(y, 1, size.length)
end

function ScenarioModderServices:_set_random_facing(entity)
   local facing = self.rng:get_int(0, 3)
   entity:add_component('mob'):turn_to(90*facing)
end

-- current algorithm returns suboptimal results for rectangular regions
function ScenarioModderServices:_get_perturbation_grid_spacing(width, length, quantity)
   local min_dimension = math.min(width, length)
   local num_divisions = math.ceil(math.sqrt(quantity))
   local grid_spacing = math.floor(min_dimension / num_divisions)
   return grid_spacing
end

return ScenarioModderServices
