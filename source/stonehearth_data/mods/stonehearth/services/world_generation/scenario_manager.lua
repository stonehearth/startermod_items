local MathFns = require 'services.world_generation.math.math_fns'
local ScenarioCategory = require 'services.world_generation.scenario_category'
local ScenarioServices = require 'services.world_generation.scenario_services'
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
   local scenarios = {}
   self._scenarios = scenarios

   -- load all the categories
   for key, value in pairs(scenario_index.categories) do
      scenarios[key] = ScenarioCategory(key, value.frequency, value.priority, self._rng)
   end

   -- load the scenarios into the categories
   for _, value in pairs(scenario_index.scenarios) do
      local item = radiant.resources.load_json(value)
      local key = item.category
      scenarios[key]:add(item)
   end
end

-- TODO: sort scenarios by priority then area
-- TODO: randomize orientation in place_entity
function ScenarioManager:place_scenarios(habitat_map, elevation_map, tile_offset_x, tile_offset_y)
   if not self._enabled then
      return
   end
   
   local rng = self._rng
   local feature_cell_size = self._feature_cell_size
   local scenarios, scenario_script, services
   local feature_width, feature_length, voxel_width, voxel_length
   local site, sites, num_sites, roll, offset_x, offset_y, habitat_type
   
   services = ScenarioServices(rng)

   scenarios = self:_select_scenarios()

   for _, properties in pairs(scenarios) do
      habitat_type = HabitatType.from_string(properties.habitat_type)
      voxel_width = properties.size.width
      voxel_length = properties.size.length
      feature_width, feature_length = self:_get_dimensions_in_feature_units(voxel_width, voxel_length)

      -- get a list of valid locations
      sites = self:_find_valid_sites(habitat_map, elevation_map, habitat_type, feature_width, feature_length)
      num_sites = #sites

      if num_sites > 0 then
         -- pick a random location
         roll = rng:get_int(1, num_sites)
         site = sites[roll]

         -- -1 to convert to C++ base 0 array coordinates
         offset_x = (site.x-1)*feature_cell_size + tile_offset_x - 1
         offset_y = (site.y-1)*feature_cell_size + tile_offset_y - 1

         services:_set_scenario_properties(properties, offset_x, offset_y)

         scenario_script = radiant.mods.load_script(properties.script)
         scenario_script:initialize(properties, services)
         
         self:_mark_scenario_site(habitat_map, site.x, site.y, feature_width, feature_length)
      end
   end
end

-- get a list of scenarios from all the categories
function ScenarioManager:_select_scenarios()
   local scenarios = self._scenarios
   local selected_scenarios = {}
   local list

   for key, _ in pairs(scenarios) do
      list = scenarios[key]:select_scenarios()
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
         -- check if block meets habitat requirements
         is_habitat_type = habitat_map:visit_block(i, j, width, length, is_target_habitat_type)
         if is_habitat_type then
            -- check if block is flat
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

return ScenarioManager
