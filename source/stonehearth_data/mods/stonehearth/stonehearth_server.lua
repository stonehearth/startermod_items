function create_service(name)
end

stonehearth = {
   constants = require 'constants'
}

radiant.events.listen(stonehearth, 'radiant:new_game', function(args)
      
      local datastore = args.datastore
      local service_creation_order = {
         'ai',
         'events',
         'calendar',
         'combat',
         'substitution',
         'personality',
         'inventory',
         'scenario',
         'population',
         'object_tracker',
         'world_generation',
         'build',
         'game_master',
         'analytics',
         'tasks',
         'terrain',
         'threads',
         'town',         
      }
      for _, service_name in ipairs(service_creation_order) do
         local path = string.format('services.%s.%s_service', service_name, service_name)
         local service = require(path)(datastore)
         stonehearth[service_name] = service
      end
   end)
return stonehearth
