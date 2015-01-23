
stonehearth = {
   constants = require 'constants'
}

local service_creation_order = {
   'threads',
   'ai',
   'events',
   'calendar',
   'game_speed',
   'tasks',
   'terrain',
   'score',
   'combat',
   'substitution',
   'personality',
   'inventory',
   'population',
   'town',
   'player',
   'spawn_region_finder',
   'dm',
   'static_scenario',
   'dynamic_scenario',
   'world_generation',
   'build',
   'game_master',
   'analytics',
   'mining',
   'town_patrol',
   'bulletin_board',
   'linear_combat',
   'interval',
   'farming',
   'trapping',
   'shepherd',
   'shop',
   'tutorial',
   'unit_control'
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
   service._sv = saved_variables:get_data()
   saved_variables:set_controller(service)
   service:initialize()
   stonehearth[name] = service
end

radiant.events.listen(stonehearth, 'radiant:init', function()
      -- global config
      radiant.terrain.set_config_file('stonehearth:terrain_block_config', true)

      -- now create all the services
      stonehearth._sv = stonehearth.__saved_variables:get_data()
      for _, name in ipairs(service_creation_order) do
         create_service(name)
      end      
   end)

return stonehearth
