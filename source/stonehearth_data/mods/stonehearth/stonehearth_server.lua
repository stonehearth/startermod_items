
stonehearth = {
   constants = require 'constants'
}

local service_creation_order = {
   'ai',
   'events',
   'calendar',
   'combat',
   'substitution',
   'personality',
   'inventory',
   'population',
   'scenario',
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

local function create_service(name)
   local path = string.format('services.%s.%s_service', name, name)
   return require(path)()
end

local function create_services(init_fn, savestate)
end

radiant.events.listen(stonehearth, 'radiant:construct', function(args)
      for _, name in ipairs(service_creation_order) do
         local service = create_service(name)
         service:initialize()
         stonehearth[name] = service
      end
   end)

radiant.events.listen(stonehearth, 'radiant:load', function(e)      
      for _, name in ipairs(service_creation_order) do
         local service = create_service(name)
         service:restore(e.savestate[name])
         stonehearth[name] = service
      end
   end)

return stonehearth
