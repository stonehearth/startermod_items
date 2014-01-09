local MathFns = require 'services.world_generation.math.math_fns'
local ScenarioCategory = require 'services.world_generation.scenario_category'
local HabitatType = require 'services.world_generation.habitat_type'
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local log = radiant.log.create_logger('world_generation')

local ScenarioManager = class()

function ScenarioManager:__init(feature_cell_size, rng)
   self._enabled = radiant.util.get_config('enable_scenarios', false)
   if not self._enabled then
      return
   end
   self._feature_cell_size = feature_cell_size
   self._rng = rng
   local scenario_index = radiant.resources.load_json('stonehearth:scenarios:scenario_index')

   self._scenarios = {}
   local scenarios = self._scenarios

   for key, value in pairs(scenario_index.categories) do
      scenarios[key] = ScenarioCategory(key, value.frequency, value.priority, self._rng)
   end

   for _, value in pairs(scenario_index.scenarios) do
      local item = radiant.resources.load_json(value)
      local key = item.category
      scenarios[key]:add(item)
   end
end

-- TODO: sort scenarios by priority then area
-- TODO: randomize orientation in place_entity
function ScenarioManager:place_scenarios(habitat_map, elevation_map, world_offset_x, world_offset_y)
   if not self._enabled then
      return
   end
   
   local rng = self._rng
   local feature_cell_size = self._feature_cell_size
   local scenarios, scenario_script
   local scenario_feature_width, scenario_feature_length, scenario_voxel_width, scenario_voxel_length
   local site, sites, num_sites, roll, offset_x, offset_y, habitat_type
   local context = {}

   context.rng = self._rng

   scenarios = self:_get_scenarios()

   for _, scenario in pairs(scenarios) do
      habitat_type = HabitatType.from_string(scenario.habitat_type)
      scenario_voxel_width = scenario.size.width
      scenario_voxel_length = scenario.size.length
      scenario_feature_width, scenario_feature_length = 
         self:_get_dimensions_in_feature_units(scenario_voxel_width, scenario_voxel_length)
      sites = self:_find_valid_sites(habitat_map, elevation_map, habitat_type, scenario_feature_width, scenario_feature_length)
      num_sites = #sites

      if num_sites > 0 then
         roll = rng:get_int(1, num_sites)
         site = sites[roll]

         -- -1 to convert to C++ base 0 array coordinates
         offset_x = (site.x-1)*feature_cell_size + world_offset_x - 1
         offset_y = (site.y-1)*feature_cell_size + world_offset_y - 1

         context.scenario = scenario

         context.place_entity = function(uri, x, y)
            x, y = self:_bounds_check(x, y, scenario_voxel_width, scenario_voxel_length)

            local entity = radiant.entities.create_entity(uri)
            radiant.terrain.place_entity(entity, Point3(x + offset_x, 1, y + offset_y))
            self:_set_random_facing(entity)
            return entity
         end

         scenario_script = radiant.mods.load_script(scenario.script)
         scenario_script:initialize(context)
         self:_mark_scenario_site(habitat_map, site.x, site.y, scenario_feature_width, scenario_feature_length)
         table.remove(sites, roll)
      end
   end
end

function ScenarioManager:_get_scenarios()
   local scenarios = self._scenarios
   local selected_scenarios = {}
   local list

   for key, _ in pairs(scenarios) do
      list = scenarios[key]:pick_scenarios()
      for _, value in pairs(list) do
         table.insert(selected_scenarios, value)
      end
   end

   return selected_scenarios
end

function ScenarioManager:_mark_scenario_site(habitat_map, i, j, width, length)
   habitat_map:set_block(i, j, width, length, HabitatType.Occupied)
end

function ScenarioManager:_find_valid_sites(habitat_map, elevation_map, habitat_type, width, length)
   local i, j, is_habitat_type, is_flat, elevation
   local sites = {}

   local is_target_habitat_type = function(value)
      return value == habitat_type
   end

   for j=1, habitat_map.height-(length-1) do
      for i=1, habitat_map.width-(width-1) do
         is_habitat_type = habitat_map:visit_block(i, j, width, length, is_target_habitat_type)
         if is_habitat_type then
            elevation = elevation_map:get(i, j)
            is_flat = elevation_map:visit_block(i, j, width, length,
               function (value)
                  return value == elevation
               end
            )
            if is_flat then
               table.insert(sites, Point2(i, j))
            end
         end
      end
   end

   return sites
end

function ScenarioManager:_get_dimensions_in_feature_units(width, length)
   return math.ceil(width/self._feature_cell_size), math.ceil(length/self._feature_cell_size)
end

function ScenarioManager:_bounds_check(x, y, width, length)
   return MathFns.bound(x, 1, width), MathFns.bound(y, 1, length)
end

function ScenarioManager:_set_random_facing(entity)
   entity:add_component('mob'):turn_to(90*self._rng:get_int(0, 3))
end

return ScenarioManager
