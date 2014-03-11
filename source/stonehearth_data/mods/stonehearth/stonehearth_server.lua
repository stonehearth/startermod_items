
stonehearth = {
   constants = require 'constants'
}

local function create_service(name)
   local path = string.format('services.%s.%s_service', name, name)         
   return require(path)()
end

radiant.events.listen(stonehearth, 'radiant:construct', function(args)     
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
      local datastore = radiant.create_datastore(stonehearth)

      for _, name in ipairs(service_creation_order) do
         local service = create_service(name)
         stonehearth[name] = service
      end
   end)

return stonehearth
