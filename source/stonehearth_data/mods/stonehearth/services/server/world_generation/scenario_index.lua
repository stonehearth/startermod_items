local HabitatManager = require 'services.server.world_generation.habitat_manager'
local ScenarioSelector = require 'services.server.world_generation.scenario_selector'
local log = radiant.log.create_logger('scenario_index')

local ScenarioIndex = class()

local activation_types = {
   immediate = true,
   revealed  = true
}

local location_types = {
   surface     = true,
   underground = true
}

function ScenarioIndex:__init(rng)
   self._rng = rng

   self:_parse_scenario_index()
end

function ScenarioIndex:get_categories()
   return self._categories
end

function ScenarioIndex:_parse_scenario_index()
   local scenario_index = radiant.resources.load_json('stonehearth:scenarios:scenario_index')
   local categories = {}
   local properties, category, error_message

   -- load all the categories
   for name, properties in pairs(scenario_index.static.categories) do
      -- parse activation type
      if not self:_is_valid_activation_type(properties.activation_type) then
         log:error('Error parsing "%s": Invalid activation_type "%s".', file, tostring(properties.activation_type))
      end
      if not self:_is_valid_location_type(properties.location_type) then
         log:error('Error parsing "%s": Invalid location_type "%s".', file, tostring(properties.location_type))
      end
      local category = {
         selector = ScenarioSelector(properties.frequency, self._rng)
      }
      -- propagate the other category properties into the category
      for key, value in pairs(properties) do
         category[key] = value
      end
      categories[name] = category
   end

   -- load the scenarios into the categories
   for _, file in pairs(scenario_index.static.scenarios) do
      properties = radiant.resources.load_json(file)

      -- parse category
      category = categories[properties.category]
      if category then
         category.selector:add(properties)
      else
         log:error('Error parsing "%s": Category "%s" has not been defined.', file, tostring(properties.category))
      end

      -- parse habitat types
      error_message = self:_parse_habitat_types(properties)
      if error_message then
         log:error('Error parsing "%s": "%s"', file, error_message)
      end
   end

   self._categories = categories
end

-- parse the habitat_types array into a set so we can index by the habitat_type
function ScenarioIndex:_parse_habitat_types(properties)
   local strings = properties.habitat_types
   local habitat_type
   local habitat_types = {}
   local error_message = nil

   for _, value in pairs(strings) do
      if HabitatManager.is_valid_habitat_type(value) then
         habitat_types[value] = value
      else
         -- concatenate multiple errors into a single string
         if error_message == nil then
            error_message = ''
         end
         error_message = string.format('%s Invalid habitat type "%s".', error_message, tostring(value))
      end
   end

   properties.habitat_types = habitat_types

   return error_message
end

function ScenarioIndex:_is_valid_activation_type(activation_type)
   return activation_types[activation_type]
end

function ScenarioIndex:_is_valid_location_type(location_type)
   return location_types[location_type]
end

function ScenarioIndex:_is_valid_habitat_type(habitat_type)
   return habitat_types[habitat_type]
end

return ScenarioIndex
