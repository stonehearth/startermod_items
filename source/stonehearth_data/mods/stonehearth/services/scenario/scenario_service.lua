local MathFns = require 'services.world_generation.math.math_fns'
local HabitatType = require 'services.world_generation.habitat_type'
local ActivationType = require 'services.scenario.activation_type'
local ScenarioSelector = require 'services.scenario.scenario_selector'
local ScenarioModderServices = require 'services.scenario.scenario_modder_services'
local Timer = require 'services.world_generation.timer'
local Point2 = _radiant.csg.Point2
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2
local _terrain = radiant._root_entity:add_component('terrain')
local log = radiant.log.create_logger('scenario_service')

local ScenarioService = class()

function ScenarioService:__init()
end

function ScenarioService:initialize()
end

function ScenarioService:restore(saved_variables)
   log:write(0, 'store not implemented for scenario service!')
end

function ScenarioService:create_new_game(feature_size, rng)
   self._feature_size = feature_size
   self._rng = rng
   self._reveal_distance = radiant.util.get_config('sight_radius', 64) * 2
   self._revealed_region = Region2()
   self._dormant_scenarios = {}
   self._last_optimized_rect_count = 10
   self._region_optimization_threshold = radiant.util.get_config('region_optimization_threshold', 1.2)

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
         log:error('Error parsing "%s": Category "%s" has not been defined.', file, tostring(properties.category))
      end

      -- parse habitat types
      properties.habitat_types, error_message = self:_parse_habitat_strings(properties.habitat_types)

      if error_message then
         log:error('Error parsing "%s": "%s"', file, error_message)
      end

      -- parse activation type
      if not ActivationType.is_valid(properties.activation_type) then
         log:error('Error parsing "%s": Invalid activation_type "%s".', file, tostring(properties.activation_type))
      end
   end

   self._categories = categories

   self:_register_events()
end

function ScenarioService:_register_events()
   -- TODO: in multiplayer, scenario service needs to reveal for all factions
   self._faction = stonehearth.population:get_faction('civ', 'stonehearth:factions:ascendancy')
   radiant.events.listen(radiant.events, 'stonehearth:very_slow_poll', self, self._on_poll)
end

function ScenarioService:_on_poll()
   self:reveal_around_entities()
end

-- hack until we make place non-static scenarios after banner placement
function ScenarioService:clear_starting_location(x, z)
   local reveal_distance = self._reveal_distance
   local region = Region2()

   region:add_cube(
      Rect2(
         -- remember +1 on max
         Point2(x-reveal_distance,   z-reveal_distance),
         Point2(x+reveal_distance+1, z+reveal_distance+1)
      )
   )

   self:reveal_region(region,
      function (scenario_properties)
         return scenario_properties.category ~= 'wildlife'
      end
   )
end

function ScenarioService:reveal_around_entities()
   local reveal_distance = self._reveal_distance
   local citizens = self._faction:get_citizens()
   local region = Region2()
   local pt, rect

   for _, entity in pairs(citizens) do
      pt = entity:add_component('mob'):get_world_grid_location()

      -- remember +1 on max
      rect = Rect2(
         Point2(pt.x-reveal_distance,   pt.z-reveal_distance),
         Point2(pt.x+reveal_distance+1, pt.z+reveal_distance+1)
      )

      region:add_cube(rect)
   end

   self:reveal_region(region)
end

function ScenarioService:reveal_region(world_space_region, activation_filter)
   local revealed_region = self._revealed_region
   local bounded_world_space_region, unrevealed_region, new_region
   local key, dormant_scenario, properties

   bounded_world_space_region = self:_bound_region_by_terrain(world_space_region)
   new_region = self:_region_to_habitat_space(bounded_world_space_region)

   unrevealed_region = new_region - revealed_region

   for rect in unrevealed_region:each_cube() do
      for j = rect.min.y, rect.max.y-1 do
         for i = rect.min.x, rect.max.x-1 do
            key = self:_get_coordinate_key(i, j)

            dormant_scenario = self._dormant_scenarios[key]
            if dormant_scenario ~= nil then
               properties = dormant_scenario.properties

               self:_mark_scenario_map(self._dormant_scenarios, nil,
                  dormant_scenario.offset_x, dormant_scenario.offset_y,
                  properties.size.width, properties.size.length
               )

               -- hack until we make place non-static scenarios after banner placement
               local activate = true
               if activation_filter then
                  activate = activation_filter(properties)
               end
               if activate then
                  local seconds = Timer.measure(
                     function()
                        self:_activate_scenario(properties, dormant_scenario.offset_x, dormant_scenario.offset_y)
                     end
                  )
                  log:info('Activated scenario "%s" in %.3fs', properties.name, seconds)
               end
            end
         end
      end
   end

   revealed_region:add_unique_region(unrevealed_region)

   local num_rects = revealed_region:get_num_rects()

   if num_rects >= self._last_optimized_rect_count * self._region_optimization_threshold then
      log:debug('Optimizing scenario region')
      revealed_region:optimize_by_oct_tree(8)
      self._last_optimized_rect_count = revealed_region:get_num_rects()
   end
end

function ScenarioService:_bound_region_by_terrain(region)
   local terrain_bounds = self:_get_terrain_region()
   return _radiant.csg.intersect_region2(region, terrain_bounds)
end

 -- this will eventually be a non-rectangular region composed of the tiles that have been generated
function ScenarioService:_get_terrain_region()
   local region = Region2()
   local bounds = _terrain:get_bounds():project_onto_xz_plane()
   region:add_cube(bounds)
   return region
end

function ScenarioService:_mark_scenario_map(map, value, world_offset_x, world_offset_y, width, length)
   local feature_x, feature_y, feature_width, feature_length, key

   feature_x, feature_y = self:_get_feature_space_coords(world_offset_x, world_offset_y)
   feature_width, feature_length = self:_get_dimensions_in_feature_units(width, length)

   -- i, j are offsets so using base 0
   for j=0, feature_length-1 do
      for i=0, feature_width-1 do
         -- dormant scenarios could also be implemented with nested tables
         key = self:_get_coordinate_key(feature_x + i, feature_y + j)
         map[key] = value
      end
   end
end

function ScenarioService:mark_scenarios(habitat_map, elevation_map, tile_offset_x, tile_offset_y)
   local rng = self._rng
   local feature_size = self._feature_size
   local feature_width, feature_length, voxel_width, voxel_length
   local selected_scenarios, site, sites, num_sites, roll, offset_x, offset_y, habitat_types
   local static_scenarios = {}

   selected_scenarios = self:_select_scenarios(habitat_map)

   for _, properties in pairs(selected_scenarios) do
      habitat_types = properties.habitat_types
      voxel_width = properties.size.width
      voxel_length = properties.size.length
      feature_width, feature_length = self:_get_dimensions_in_feature_units(voxel_width, voxel_length)

      -- get a list of valid locations
      sites, num_sites = self:_find_valid_sites(habitat_map, elevation_map, habitat_types, feature_width, feature_length)

      if num_sites > 0 then
         -- pick a random location
         roll = rng:get_int(1, num_sites)
         site = sites[roll]

         -- these are in C++ base 0 array coordinates
         offset_x = (site.x-1)*feature_size + tile_offset_x
         offset_y = (site.y-1)*feature_size + tile_offset_y

         local scenario_info = {
            properties = properties,
            offset_x = offset_x,
            offset_y = offset_y
         }

         if properties.activation_type == ActivationType.static then
            table.insert(static_scenarios, scenario_info)
         else
            self:_mark_scenario_map(self._dormant_scenarios, scenario_info, offset_x, offset_y, voxel_width, voxel_length)
         end

         self:_mark_habitat_map(habitat_map, site.x, site.y, feature_width, feature_length)

         if properties.unique then
            self:_remove_scenario(properties)
         end
      end
   end

   return static_scenarios
end

function ScenarioService:place_static_scenarios(scenarios)
   for _, scenario_info in pairs(scenarios) do
      if scenario_info.properties.activation_type == ActivationType.static then
         self:_activate_scenario(scenario_info.properties, scenario_info.offset_x, scenario_info.offset_y)
      end
   end
end

-- TODO: randomize orientation in place_entity
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
   local num_sites = 0

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
               num_sites = num_sites + 1
               sites[num_sites] = Point2(i, j)
            end
         end
      end
   end

   return sites, num_sites
end

-- get a list of scenarios from all the categories
function ScenarioService:_select_scenarios(habitat_map)
   local categories = self._categories
   local scenarios = {}
   local list

   for key, _ in pairs(categories) do
      list = categories[key]:select_scenarios(habitat_map)
      for _, item in pairs(list) do
         table.insert(scenarios, item)
      end
   end

   self:_sort_scenarios(scenarios)

   return scenarios
end

-- order first by priority, then by area, then by weight
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
      if area_a ~= area_b then
         -- larger area sorted to lower index
         return area_a > area_b 
      end

      return a.weight > b.weight
   end

   table.sort(scenarios, comparator)
end

function ScenarioService:_remove_scenario(scenario)
   local name = scenario.name
   local category = scenario.category
   self._categories[category]:remove(name)
end

function ScenarioService:_mark_habitat_map(habitat_map, i, j, width, length)
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
   local feature_size = self._feature_size
   local min, max

   min = Point2(math.floor(rect.min.x/feature_size),
                math.floor(rect.min.y/feature_size))

   if rect:get_area() == 0 then
      max = min
   else
      max = Point2(math.ceil(rect.max.x/feature_size),
                   math.ceil(rect.max.y/feature_size))
   end

   return Rect2(min, max)
end

function ScenarioService:_get_dimensions_in_feature_units(width, length)
   local feature_size = self._feature_size
   return math.ceil(width/feature_size), math.ceil(length/feature_size)
end

function ScenarioService:_get_feature_space_coords(x, y)
   local feature_size = self._feature_size
   return math.floor(x/feature_size),
          math.floor(y/feature_size)
end

function ScenarioService:_get_coordinate_key(x, y)
   return string.format('%d,%d', x, y)
end

-- parse the array into a set so we can index by the habitat_type
function ScenarioService:_parse_habitat_strings(strings)
   local habitat_type
   local habitat_types = {}
   local error_message = nil

   for _, value in pairs(strings) do
      if HabitatType.is_valid(value) then
         habitat_types[value] = value
      else
         -- concatenate multiple errors into a single string
         if error_message == nil then
            error_message = ''
         end
         error_message = string.format('%s Invalid habitat type "%s".', error_message, tostring(value))
      end
   end

   return habitat_types, error_message
end

return ScenarioService
