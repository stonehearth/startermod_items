
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
   'farming'  
}

local function create_service(name)
   local path = string.format('services.%s.%s_service', name, name)
   return require(path)()
end

radiant.events.listen(stonehearth, 'radiant:construct', function(args)
      for _, name in ipairs(service_creation_order) do
         local service = create_service(name)
         stonehearth[name] = service
      end
   end)

radiant.events.listen(stonehearth, 'radiant:load', function(e)
      local savestate = e.savestate
      for _, name in ipairs(service_creation_order) do
         local service = create_service(name)
         if service.load then
            service:load(savestate[name])
         end

         stonehearth[name] = service
      end
   end)

return stonehearth
