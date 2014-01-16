local MathFns = require 'services.world_generation.math.math_fns'
local HabitatType = require 'services.world_generation.habitat_type'
local ActivationType = require 'services.scenario.activation_type'
local ScenarioSelector = require 'services.scenario.scenario_selector'
local ScenarioModderServices = require 'services.scenario.scenario_modder_services'
local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local Region2 = _radiant.csg.Region2
local Rect2 = _radiant.csg.Rect2
local log = radiant.log.create_logger('scenario_service')

local ScenarioService = class()

function ScenarioService:__init()
end

function ScenarioService:initialize(feature_cell_size, rng)
   self._feature_cell_size = feature_cell_size
   self._rng = rng
   self._revealed_region = Region2()
   self._dormant_scenarios = {}

   local scenario_index = radiant.resources.load_json('stonehearth:scenarios:scenario_index')
   local categories = {}
   local properties, category, error_message

   -- load all the categories
   for name, properties in pairs(scenario_index.categories) do
      categories[name] = ScenarioSelector(properties.frequency, properties.priority, self._rng)
   end

   -- load the scenarios into the categories
   for _, file in pairs(scenario_index.scenarios) do
      properties = radiant.resources.load_json(file)

      -- parse category
      category = categories[properties.category]
      if category then
         category:add(properties)
      else
         log:error('Error parsing "%s": Category "%s" has not been defined.', file, _wrap_nil(properties.category))
      end

      -- parse habitat types
      properties.habitat_types, error_message = self:_parse_habitat_strings(properties.habitat_types)

      if error_message then
         log:error('Error parsing "%s": "%s"', file, error_message)
      end

      -- parse activation type
      if not ActivationType.is_valid(properties.activation_type) then
         log:error('Error parsing "%s": Invalid activation_type "%s".', file, _wrap_nil(properties.activation_type))
      end
   end

   self._categories = categories
end

function ScenarioService:reveal_region(world_space_region)
   local unrevealed_region, new_region, num_rects, rect, key, dormant_scenario, properties

   new_region = self:_region_to_habitat_space(world_space_region)

   -- this gets expensive as self._revealed_region grows
   -- we could just forget the subtraction and just iterate over the incoming region instead
   unrevealed_region = new_region - self._revealed_region
   num_rects = unrevealed_region:get_num_rects()

   -- use c++ base 0 array indexing
   for n=0, num_rects-1 do
      rect = unrevealed_region:get_rect(n)

      for j = rect.min.y, rect.max.y-1 do
         for i = rect.min.x, rect.max.x-1 do
            key = self:_get_coordinate_key(i, j)

            dormant_scenario = self._dormant_scenarios[key]
            if dormant_scenario ~= nil then
               properties = dormant_scenario.properties

               self:_mark_dormant_scenario_map(nil,
                  dormant_scenario.offset_x, dormant_scenario.offset_y,
                  properties.size.width, properties.size.length
               )

               self:_activate_scenario(properties, dormant_scenario.offset_x, dormant_scenario.offset_y)
            end
         end
      end
   end

   self._revealed_region:add_region(unrevealed_region)
end

function ScenarioService:_mark_dormant_scenario_map(value, world_offset_x, world_offset_y, width, length)
   local feature_x, feature_y, feature_width, feature_length, key

   feature_x, feature_y = self:_get_feature_space_coords(world_offset_x, world_offset_y)
   feature_width, feature_length = self:_get_dimensions_in_feature_units(width, length)

   -- i, j are offsets so using base 0
   for j=0, feature_length-1 do
      for i=0, feature_width-1 do
         key = self:_get_coordinate_key(feature_x + i, feature_y + j)
         self._dormant_scenarios[key] = value
      end
   end
end

-- TODO: randomize orientation in place_entity
function ScenarioService:place_scenarios(habitat_map, elevation_map, tile_offset_x, tile_offset_y)
   local rng = self._rng
   local feature_cell_size = self._feature_cell_size
   local feature_width, feature_length, voxel_width, voxel_length
   local scenarios, site, sites, num_sites, roll, offset_x, offset_y, habitat_types
   
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

         -- these are in C++ base 0 array coordinates
         offset_x = (site.x-1)*feature_cell_size + tile_offset_x
         offset_y = (site.y-1)*feature_cell_size + tile_offset_y

         if properties.activation_type == ActivationType.static then
            self:_activate_scenario(properties, offset_x, offset_y)
         else
            local dormant_scenario = {
               properties = properties,
               offset_x = offset_x,
               offset_y = offset_y
            }
            self:_mark_dormant_scenario_map(dormant_scenario, offset_x, offset_y, voxel_width, voxel_length)
         end

         self:_mark_site_occupied(habitat_map, site.x, site.y, feature_width, feature_length)

         if properties.unique then
            self:_remove_scenario(properties)
         end
      end
   end
end

function ScenarioService:_activate_scenario(properties, offset_x, offset_y)
   local services, scenario_script

   services = ScenarioModderServices(self._rng)
   services:_set_scenario_properties(properties, offset_x, offset_y)

   scenario_script = radiant.mods.load_script(properties.script)
   scenario_script()
   scenario_script:initialize(properties, services)
end

function ScenarioService:_find_valid_sites(habitat_map, elevation_map, habitat_types, width, length)
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
function ScenarioService:_select_scenarios()
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
function ScenarioService:_sort_scenarios(scenarios)
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

function ScenarioService:_remove_scenario(scenario)
   local name = scenario.name
   local category = scenario.category
   self._categories[category]:remove(name)
end

function ScenarioService:_mark_site_occupied(habitat_map, i, j, width, length)
   habitat_map:set_block(i, j, width, length, HabitatType.occupied)
end

function ScenarioService:_region_to_habitat_space(region)
   local new_region = Region2()
   local num_rects = region:get_num_rects()
   local rect, new_rect

   -- use c++ base 0 array indexing
   for i=0, num_rects-1 do
      rect = region:get_rect(i)
      new_rect = self:_rect_to_habitat_space(rect)
      -- can't use add_unique_cube because of quantization to reduced coordinate space
      new_region:add_cube(new_rect)
   end

   return new_region
end

function ScenarioService:_rect_to_habitat_space(rect)
   local feature_cell_size = self._feature_cell_size
   local min, max

   min = Point2(math.floor(rect.min.x/feature_cell_size),
                math.floor(rect.min.y/feature_cell_size))

   if rect:get_area() == 0 then
      max = min
   else
      max = Point2(math.ceil(rect.max.x/feature_cell_size),
                   math.ceil(rect.max.y/feature_cell_size))
   end

   return Rect2(min, max)
end

function ScenarioService:_get_dimensions_in_feature_units(width, length)
   local feature_cell_size = self._feature_cell_size
   return math.ceil(width/feature_cell_size), math.ceil(length/feature_cell_size)
end

function ScenarioService:_get_feature_space_coords(x, y)
   local feature_cell_size = self._feature_cell_size
   return math.floor(x/feature_cell_size),
          math.floor(y/feature_cell_size)
end

function ScenarioService:_get_coordinate_key(x, y)
   return string.format('%d,%d', x, y)
end

-- returns a table with the set of valid habitat_types
function ScenarioService:_parse_habitat_strings(strings)
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
         error_message = string.format('%s Invalid habitat type "%s".', error_message, _wrap_nil(value))
      end
   end

   return habitat_types, error_message
end

function _wrap_nil(value)
   if value ~= nil then
      return value
   end
   return 'nil'
end

return ScenarioService
