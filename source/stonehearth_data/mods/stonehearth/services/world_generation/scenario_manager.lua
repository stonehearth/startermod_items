local MathFns = require 'services.world_generation.math.math_fns'
local ScenarioCategory = require 'services.world_generation.scenario_category'
local ScenarioServices = require 'services.world_generation.scenario_services'
local HabitatType = require 'services.world_generation.habitat_type'
local StringFns = require 'services.world_generation.string_fns'
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local log = radiant.log.create_logger('world_generation')

local ScenarioManager = class()

function ScenarioManager:__init(feature_cell_size, rng)
   self._feature_cell_size = feature_cell_size
   self._rng = rng

   local scenario_index = radiant.resources.load_json('stonehearth:scenarios:scenario_index')
   local categories = {}
   local scenario, scenario_category, error_message

   -- load all the categories
   for name, properties in pairs(scenario_index.categories) do
      categories[name] = ScenarioCategory(name, properties.frequency, properties.priority, self._rng)
   end

   -- load the scenarios into the categories
   for _, file in pairs(scenario_index.scenarios) do
      scenario = radiant.resources.load_json(file)
      scenario.habitat_types, error_message = self:_parse_habitat_strings(scenario.habitat_types)

      if error_message then
         log:error('Error parsing %s: %s', file, error_message)
      end

      scenario_category = categories[scenario.category]
      if scenario_category then
         scenario_category:add(scenario)
      else
         log:error('Error parsing %s: Invalid category "%s".', file, scenario.category)
      end
   end

   self._categories = categories
end

-- returns a table with the set of valid habitat_types
function ScenarioManager:_parse_habitat_strings(strings)
   local habitat_type
   local habitat_types = {}
   local error_message = nil

   for _, value in pairs(strings) do
      if HabitatType.is_valid(value) then
         habitat_types[value] = true
      else
         -- concatenate multiple errors into a single string
         if error_message == nil then
            error_message = ''
         end
         error_message = string.format('%s Invalid habitat type "%s".', error_message, value)
      end
   end

   return habitat_types, error_message
end

-- TODO: sort scenarios by priority then area
-- TODO: randomize orientation in place_entity
function ScenarioManager:place_scenarios(habitat_map, elevation_map, tile_offset_x, tile_offset_y)
   local rng = self._rng
   local feature_cell_size = self._feature_cell_size
   local scenarios, scenario_script, services
   local feature_width, feature_length, voxel_width, voxel_length
   local site, sites, num_sites, roll, offset_x, offset_y, habitat_types
   
   services = ScenarioServices(rng)

   scenarios = self:_select_scenarios()

   for _, properties in pairs(scenarios) do
      habitat_types = properties.habitat_types
      voxel_width = properties.size.width
      voxel_length = properties.size.length
      feature_width, feature_length = self:_get_dimensions_in_feature_units(voxel_width, voxel_length)

      -- get a list of valid locations
      sites = self:_find_valid_sites(habitat_map, elevation_map, habitat_types, feature_width, feature_length)
      num_sites = #sites

      if num_sites > 0 then
         -- pick a random location
         roll = rng:get_int(1, num_sites)
         site = sites[roll]

         -- -1 to convert to C++ base 0 array coordinates
         offset_x = (site.x-1)*feature_cell_size + tile_offset_x - 1
         offset_y = (site.y-1)*feature_cell_size + tile_offset_y - 1

         -- set parameters that are needed by ScenarioServices
         services:_set_scenario_properties(properties, offset_x, offset_y)

         scenario_script = radiant.mods.load_script(properties.script)
         scenario_script:initialize(properties, services)
         
         self:_mark_site_occupied(habitat_map, site.x, site.y, feature_width, feature_length)

         if properties.unique then
            self:_remove_scenario(properties)
         end
      end
   end
end

function ScenarioManager:_find_valid_sites(habitat_map, elevation_map, habitat_types, width, length)
   local i, j, is_habitat_type, is_flat, elevation
   local sites = {}

   local is_target_habitat_type = function(value)
      local found = habitat_types[value]
      return found
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

-- get a list of scenarios from all the categories
function ScenarioManager:_select_scenarios()
   local categories = self._categories
   local scenarios = {}
   local list

   for key, _ in pairs(categories) do
      list = categories[key]:select_scenarios()
      for _, item in pairs(list) do
         table.insert(scenarios, item)
      end
   end

   self:_sort_scenarios(scenarios)

   return scenarios
end

-- order first by priority then by area
function ScenarioManager:_sort_scenarios(scenarios)
   local comparator = function(a, b)
      local category_a = a.category
      local category_b = b.category

      if category_a ~= category_b then
         local categories = self._categories
         local priority_a = categories[category_a].priority
         local priority_b = categories[category_b].priority
         -- higher priority sorted to lower index
         return priority_a > priority_b
      end

      local area_a = a.size.width * a.size.length
      local area_b = b.size.width * b.size.length
      -- larger area sorted to lower index
      return area_a > area_b 
   end

   table.sort(scenarios, comparator)
end

function ScenarioManager:_remove_scenario(scenario)
   local name = scenario.name
   local category = scenario.category
   self._categories[category]:remove(name)
end

function ScenarioManager:_mark_site_occupied(habitat_map, i, j, width, length)
   habitat_map:set_block(i, j, width, length, HabitatType.occupied)
end

function ScenarioManager:_get_dimensions_in_feature_units(width, length)
   return math.ceil(width/self._feature_cell_size), math.ceil(length/self._feature_cell_size)
end

return ScenarioManager
