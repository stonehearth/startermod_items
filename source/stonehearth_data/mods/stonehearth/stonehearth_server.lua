
stonehearth = {
   constants = require 'constants'
}

local service_creation_order = {
   'ai',
   'events',
   'calendar',
   'game_speed',
   'score',
   'combat',
   'substitution',
   'personality',
   'inventory',
   'population',
   'spawn_region_finder',
   'dm',
   'static_scenario',
   'dynamic_scenario',
   'object_tracker',
   'world_generation',
   'build',
   'game_master',
   'analytics',
   'tasks',
   'terrain',
   'threads',
   'town',
   'town_defense',
   'farming'
}

local function create_service(name)
   local path = string.format('services.server.%s.%s_service', name, name)
   local service = require(path)()

   local saved_variables = stonehearth._sv[name]
   if not saved_variables then
      saved_variables = radiant.create_datastore()
      stonehearth._sv[name] = saved_variables
   end
   service.__saved_variables = saved_variables
   service:initialize()
   stonehearth[name] = service
end

radiant.events.listen(stonehearth, 'radiant:init', function()
      stonehearth._sv = stonehearth.__saved_variables:get_data()
      for _, name in ipairs(service_creation_order) do
         create_service(name)
      end
   end)

return stonehearth
